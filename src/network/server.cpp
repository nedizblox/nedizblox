#include "server.hpp"

#include "tcp_framing.hpp"

#include "packets.hpp"

#include "core/crypto.hpp"
#include "core/logger.hpp"

#include <algorithm>
#include <cstring>
#include <format>

// NOLINTBEGIN(bugprone-unused-return-value)

namespace net {

Server::Server(uint16_t port) :
    m_tcpAcceptor(m_context, tcp::endpoint(tcp::v4(), port)),
    m_udpSocket(m_context, udp::endpoint(udp::v4(), port)) {

    boost::system::error_code ec;

    m_tcpAcceptor.non_blocking(true, ec);
    m_udpSocket.non_blocking(true, ec);

    core::logger::info(std::format("Server initialized on port {}", port));

    m_acceptThread = std::thread(&Server::acceptLoop, this);
    m_statsThread = std::thread(&Server::statsLoop, this);
}

Server::~Server() { stop(); }

void Server::update() {
    if (!m_running || !m_udpSocket.is_open())
        return;

    std::array<uint8_t, 1400> udpBuffer;
    udp::endpoint remote;
    boost::system::error_code error;

    size_t bytes = m_udpSocket.receive_from(boost::asio::buffer(udpBuffer), remote, 0, error);

    if (error == boost::asio::error::operation_aborted)
        return;

    if (error && error != boost::asio::error::message_size)
        return;

    m_bytesReceived += bytes;

    if (bytes == sizeof(packets::PingPacket)) {
        packets::PingPacket ping{};
        std::memcpy(&ping, udpBuffer.data(), sizeof(ping));

        if (ping.magic == packets::kPingMagic) {
            packets::PongPacket pong{};
            pong.clientTimeUs = ping.clientTimeUs;

            boost::system::error_code sendEc;
            m_udpSocket.send_to(boost::asio::buffer(&pong, sizeof(pong)), remote, 0, sendEc);
            if (!sendEc) {
                m_bytesSent += sizeof(pong);
            }
            return;
        }
    }

    if (bytes == sizeof(packets::ClientUdpConnectPacket)) {
        packets::ClientUdpConnectPacket hello{};
        std::memcpy(&hello, udpBuffer.data(), sizeof(hello));

        {
            std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
            auto clientIt = m_tcpClients.find(hello.playerId);
            if (clientIt == m_tcpClients.end() || clientIt->second.sessionToken != hello.sessionToken) {
                return;
            }
        }

        m_udpPlayerIds[remote] = hello.playerId;

        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_clients.insert(remote);
        }

        return;
    }

    if (bytes < sizeof(packets::PhysicsStepPacket))
        return;

    packets::PhysicsStepPacket header{};
    std::memcpy(&header, udpBuffer.data(), sizeof(header));

    {
        std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
        if (m_tcpClients.find(header.senderPlayerId) == m_tcpClients.end()) {
            return;
        }
    }

    auto endpointIt = m_udpPlayerIds.find(remote);
    if (endpointIt == m_udpPlayerIds.end() || endpointIt->second != header.senderPlayerId) {
        return;
    }

    size_t expectedSize = sizeof(packets::PhysicsStepPacket)
                          + (static_cast<size_t>(header.objectCount) * sizeof(packets::PhysicalObjectState));
    if (bytes != expectedSize)
        return;

    const packets::PhysicalObjectState* statesPtr
        = reinterpret_cast<const packets::PhysicalObjectState*>(udpBuffer.data() + sizeof(header));
    std::vector<packets::PhysicalObjectState> objects(statesPtr, statesPtr + header.objectCount);

    {
        std::lock_guard<std::mutex> lock(m_objectOwnersMutex);
        auto now = std::chrono::steady_clock::now();
        for (const auto& object : objects) {
            m_objectOwners[object.networkId] = ObjectOwnership{header.senderPlayerId, now};
        }
    }

    if (m_physicsUpdateCallback) {
        m_physicsUpdateCallback(header.senderPlayerId, objects);
    }

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (const auto& clientEndpoint : m_clients) {
            boost::system::error_code ec;
            m_udpSocket.send_to(boost::asio::buffer(udpBuffer.data(), bytes), clientEndpoint, 0, ec);
            if (!ec) {
                m_bytesSent += bytes;
            }
        }
    }
}

void Server::stop() {
    if (!m_running)
        return;
    m_running = false;

    boost::system::error_code ec;

    if (m_udpSocket.is_open()) {
        m_udpSocket.cancel(ec);
        m_udpSocket.shutdown(udp::socket::shutdown_both, ec);
        m_udpSocket.close(ec);
    }

    if (m_tcpAcceptor.is_open()) {
        m_tcpAcceptor.close(ec);
    }

    kickAll("Server has shutdown");

    m_context.stop();

    if (m_acceptThread.joinable()) {
        m_acceptThread.join();
    }

    if (m_statsThread.joinable()) {
        m_statsThread.join();
    }

    for (auto& thread : m_tcpClientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void Server::setMapData(std::vector<uint8_t> mapData) {
    std::lock_guard<std::mutex> lock(m_mapDataMutex);
    m_mapData = std::move(mapData);
}

bool Server::isOwned(uint32_t networkId) const {
    std::lock_guard<std::mutex> lock(m_objectOwnersMutex);

    auto it = m_objectOwners.find(networkId);
    if (it == m_objectOwners.end())
        return false;

    return (std::chrono::steady_clock::now() - it->second.lastUpdate) < kOwnershipTimeout;
}

void Server::broadcastPhysicsState(const std::vector<packets::PhysicalObjectState>& objects) {
    constexpr size_t kMaxObjectsPerPacket = (kMaxUdpDatagramSize - sizeof(packets::PhysicsStepPacket))
                                            / sizeof(packets::PhysicalObjectState);

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    if (m_clients.empty())
        return;

    for (size_t offset = 0; offset < objects.size(); offset += kMaxObjectsPerPacket) {
        size_t count = std::min(kMaxObjectsPerPacket, objects.size() - offset);

        packets::PhysicsStepPacket header{};
        header.senderPlayerId = 0;
        header.objectCount = static_cast<uint32_t>(count);

        std::vector<uint8_t> buffer(sizeof(header) + count * sizeof(packets::PhysicalObjectState));
        std::memcpy(buffer.data(), &header, sizeof(header));
        std::memcpy(buffer.data() + sizeof(header), objects.data() + offset, count * sizeof(packets::PhysicalObjectState));

        for (const auto& clientEndpoint : m_clients) {
            boost::system::error_code ec;
            m_udpSocket.send_to(boost::asio::buffer(buffer), clientEndpoint, 0, ec);
            if (!ec) {
                m_bytesSent += buffer.size();
            }
        }
    }
}

void Server::handleTcpClient(TcpClient client) {
    while (m_running) {
        std::vector<uint8_t> payload;
        boost::system::error_code ec;

        if (!tcpframing::readFramed(*client.socket, payload, ec)) {
            break;
        }

        m_bytesReceived += sizeof(packets::TcpMessageHeader) + payload.size();

        if (payload.size() < sizeof(packets::PacketHeader))
            continue;

        auto type = static_cast<packets::PacketType>(payload[0]);

        if (type == packets::PacketType::PlayerAccept || type == packets::PacketType::PlayerJoined
            || type == packets::PacketType::PlayerLeft || type == packets::PacketType::PlayerKick) {
            kickClient(*client.socket, "Detected server packet spoofing");
            break;
        }

        bool isValid = false;

        switch (type) {
        case packets::PacketType::PlayerInfo:
            if (payload.size() == sizeof(packets::PlayerInfoPacket))
                isValid = true;
            break;
        default:
            break;
        }

        if (!isValid)
            continue;

        std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
        for (auto& [id, other] : m_tcpClients) {
            if (id == client.playerId)
                continue;

            boost::system::error_code sendEc;
            tcpframing::writeFramed(*other.socket, payload.data(), payload.size(), sendEc);

            if (sendEc) {
                core::logger::err("Server TCP relay: " + sendEc.message());
            } else {
                m_bytesSent += sizeof(packets::TcpMessageHeader) + payload.size();
            }
        }
    }

    boost::system::error_code ec;
    client.socket->shutdown(tcp::socket::shutdown_both, ec);
    client.socket->close(ec);

    {
        std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
        m_tcpClients.erase(client.playerId);
    }

    {
        std::lock_guard<std::mutex> lock(m_objectOwnersMutex);
        for (auto it = m_objectOwners.begin(); it != m_objectOwners.end();) {
            if (it->second.ownerPlayerId == client.playerId) {
                it = m_objectOwners.erase(it);
            } else {
                it++;
            }
        }
    }

    core::logger::info(std::format("Player \"{}\" (id {}) disconnected", client.nickname, client.playerId));

    packets::PlayerLeftPacket left{};
    left.playerId = client.playerId;
    broadcastReliable(&left, sizeof(left));
}

void Server::broadcastReliable(const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
    for (auto& [id, client] : m_tcpClients) {
        boost::system::error_code ec;
        tcpframing::writeFramed(*client.socket, data, size, ec);

        if (ec) {
            core::logger::err("BroadcastReliable: " + ec.message());
        } else {
            m_bytesSent += sizeof(packets::TcpMessageHeader) + size;
        }
    }
}

void Server::acceptLoop() {
    while (m_running) {
        auto socket = std::make_shared<tcp::socket>(m_tcpAcceptor.get_executor());

        boost::system::error_code ec;
        m_tcpAcceptor.accept(*socket, ec);

        if (ec == boost::asio::error::would_block) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (ec) {
            if (m_running) {
                core::logger::err("TCP acceptor stopped: " + ec.message());
            }
            break;
        }

        std::vector<uint8_t> payload;
        if (!tcpframing::readFramed(*socket, payload, ec) || payload.size() != sizeof(packets::PlayerInfoPacket)) {
            core::logger::err("TCP client failed handshake: " + ec.message());
            socket->close(ec);
            continue;
        }
        m_bytesReceived += sizeof(packets::TcpMessageHeader) + payload.size();

        packets::PlayerInfoPacket info{};
        std::memcpy(&info, payload.data(), sizeof(info));
        info.nickname[sizeof(info.nickname) - 1] = '\0';

        auto version = core::crypto::encodeVersion(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
        if (info.clientVersion != version) {
            uint32_t clientMajor, clientMinor, clientPatch = 0;
            core::crypto::decodeVersion(info.clientVersion, clientMajor, clientMinor, clientPatch);

            kickClient(
                *socket,
                std::format(
                    "Your client version doesn't match the server version ({}.{}.{} != {}.{}.{})",
                    clientMajor, clientMinor, clientPatch, PROJECT_VERSION_MAJOR,
                    PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH));
            continue;
        }

        std::string nicknameStr(info.nickname);
        if (nicknameStr.length() > 12 || nicknameStr.empty()) {
            kickClient(*socket, "Nickname is too long (maximum 12 symbols) or empty");
            continue;
        } else if (nicknameStr.length() < 5) {
            kickClient(*socket, "Nickname is too small (minimum 5 symbols)");
            continue;
        }

        bool nicknameExists = false;
        {
            std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
            for (const auto& [id, client] : m_tcpClients) {
                if (client.nickname == nicknameStr) {
                    nicknameExists = true;
                    break;
                }
            }
        }

        if (nicknameExists) {
            kickClient(*socket, "This nickname is already taken");
            continue;
        }

        uint32_t playerId = m_nextPlayerId.fetch_add(1);
        uint64_t sessionToken = core::crypto::generateToken();

        packets::PlayerAcceptPacket accept{};
        accept.playerId = playerId;
        accept.sessionToken = sessionToken;
        if (!tcpframing::writeFramed(*socket, &accept, sizeof(accept), ec)) {
            core::logger::err("Failed to send PlayerAcceptPacket: " + ec.message());
            socket->close(ec);
            continue;
        }
        m_bytesSent += sizeof(packets::TcpMessageHeader) + sizeof(accept);

        packets::InitialStatePacket initialState{};

        std::vector<uint8_t> stateBuffer;
        {
            std::lock_guard<std::mutex> lock(m_tcpClientsMutex);

            initialState.playerCount = static_cast<uint32_t>(m_tcpClients.size());

            stateBuffer.resize(
                sizeof(packets::InitialStatePacket) + (initialState.playerCount * sizeof(packets::OldPlayerInfo)));
            std::memcpy(stateBuffer.data(), &initialState, sizeof(initialState));

            size_t offset = sizeof(packets::InitialStatePacket);
            for (const auto& [id, oldClient] : m_tcpClients) {
                packets::OldPlayerInfo oldInfo{};
                oldInfo.playerId = oldClient.playerId;
                std::strncpy(oldInfo.nickname, oldClient.nickname.c_str(), sizeof(oldInfo.nickname) - 1);
                oldInfo.position = oldClient.lastPosition;

                std::memcpy(stateBuffer.data() + offset, &oldInfo, sizeof(oldInfo));
                offset += sizeof(oldInfo);
            }
        }

        if (!tcpframing::writeFramed(*socket, stateBuffer.data(), stateBuffer.size(), ec)) {
            socket->close(ec);
            continue;
        }
        m_bytesSent += sizeof(packets::TcpMessageHeader) + stateBuffer.size();

        {
            std::lock_guard<std::mutex> lock(m_mapDataMutex);
            if (!tcpframing::writeFramed(*socket, m_mapData.data(), m_mapData.size(), ec)) {
                socket->close(ec);
                continue;
            }
            m_bytesSent += sizeof(packets::TcpMessageHeader) + m_mapData.size();
        }

        core::logger::info(
            std::format(
                "Player \"{}\" connected, assigned id {} ({}:{})", info.nickname, playerId,
                socket->remote_endpoint().address().to_string(), socket->remote_endpoint().port()));

        TcpClient client{socket, playerId, sessionToken, std::string(info.nickname)};
        {
            std::lock_guard<std::mutex> lock(m_tcpClientsMutex);
            m_tcpClients.emplace(playerId, client);
        }

        packets::PlayerJoinedPacket joined{};
        joined.playerId = playerId;
        std::strncpy(joined.nickname, info.nickname, sizeof(joined.nickname) - 1);
        broadcastReliable(&joined, sizeof(joined));

        std::thread(&Server::handleTcpClient, this, client).detach();
    }
}

void Server::kickClient(tcp::socket& socket, const std::string& reason) {
    core::logger::info(
        std::format(
            "Rejecting client ({}:{}): {}", socket.remote_endpoint().address().to_string(),
            socket.remote_endpoint().port(), reason));

    packets::PlayerKickPacket kickPacket{};
    std::strncpy(kickPacket.reason, reason.c_str(), sizeof(kickPacket.reason) - 1);

    boost::system::error_code kickEc;
    tcpframing::writeFramed(socket, &kickPacket, sizeof(kickPacket), kickEc);

    if (kickEc) {
        core::logger::err("Kick: " + kickEc.message());
    } else {
        m_bytesSent += sizeof(packets::TcpMessageHeader) + sizeof(kickPacket);
    }

    socket.close(kickEc);
}

void Server::kickAll(const std::string& reason) {
    packets::PlayerKickPacket kickPacket{};
    std::strncpy(kickPacket.reason, reason.c_str(), sizeof(kickPacket.reason) - 1);

    std::lock_guard<std::mutex> lock(m_tcpClientsMutex);

    for (auto& [id, client] : m_tcpClients) {
        auto socket = client.socket;

        core::logger::info(
            std::format(
                "Rejecting client ({}:{}): {}", socket->remote_endpoint().address().to_string(),
                socket->remote_endpoint().port(), reason));

        boost::system::error_code kickEc;
        tcpframing::writeFramed(*client.socket, &kickPacket, sizeof(kickPacket), kickEc);

        if (kickEc) {
            core::logger::err("Kick: " + kickEc.message());
        } else {
            m_bytesSent += sizeof(packets::TcpMessageHeader) + sizeof(kickPacket);
        }

        client.socket->shutdown(tcp::socket::shutdown_both, kickEc);
        client.socket->close(kickEc);
    }
}

void Server::statsLoop() {
    auto lastTime = std::chrono::steady_clock::now();
    uint64_t lastSent = m_bytesSent.load();
    uint64_t lastReceived = m_bytesReceived.load();

    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!m_running)
            break;

        auto now = std::chrono::steady_clock::now();
        double elapsedSec = std::chrono::duration<double>(now - lastTime).count();
        if (elapsedSec < 1.0)
            continue;

        uint64_t sentNow = m_bytesSent.load();
        uint64_t receivedNow = m_bytesReceived.load();

        m_sentKBps = (static_cast<double>(sentNow - lastSent) / 1024.0) / elapsedSec;
        m_recvKBps = (static_cast<double>(receivedNow - lastReceived) / 1024.0) / elapsedSec;

        lastSent = sentNow;
        lastReceived = receivedNow;
        lastTime = now;
    }
}

} // namespace net

// NOLINTEND(bugprone-unused-return-value)