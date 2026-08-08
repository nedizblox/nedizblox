#include "client.hpp"

#include "tcp_framing.hpp"

#include "core/crypto.hpp"
#include "core/logger.hpp"

#include <chrono>
#include <cstring>
#include <format>

// NOLINTBEGIN(bugprone-unused-return-value)

namespace net {

Client::Client(const std::string& host, uint16_t port, const std::string& nickname) :
    m_udpSocket(m_ioContext, udp::v4()), m_tcpSocket(m_ioContext) {
    tcp::resolver tcpResolver(m_ioContext);
    auto tcpEndpoints = tcpResolver.resolve(host, std::to_string(port));

    boost::system::error_code ec;
    boost::asio::connect(m_tcpSocket, tcpEndpoints, ec);

    if (ec) {
        throw std::runtime_error("Client TCP connect failed: " + ec.message());
    }

    packets::PlayerInfoPacket info{};
    std::strncpy(info.nickname, nickname.c_str(), sizeof(info.nickname) - 1);
    info.clientVersion
        = core::crypto::encodeVersion(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);

    if (!tcpframing::writeFramed(m_tcpSocket, &info, sizeof(info), ec)) {
        throw std::runtime_error("Client failed to send PlayerInfoPacket: " + ec.message());
    }
    m_bytesSent += sizeof(packets::TcpMessageHeader) + sizeof(info);

    std::vector<uint8_t> reply;
    if (!tcpframing::readFramed(m_tcpSocket, reply, ec)) {
        throw std::runtime_error("Client did not receive TCP handshake response: " + ec.message());
    }
    m_bytesReceived += sizeof(packets::TcpMessageHeader) + reply.size();

    if (reply.size() == sizeof(packets::PlayerKickPacket)) {
        packets::PlayerKickPacket kick{};
        std::memcpy(&kick, reply.data(), sizeof(kick));
        throw std::runtime_error(std::format("Client kicked from server: {}", kick.reason));
    }

    if (reply.size() != sizeof(packets::PlayerAcceptPacket)) {
        throw std::runtime_error(
            std::format(
                "Client did not receive a valid PlayerAcceptPacket: Size mismatch (Expected {}, "
                "got {})",
                sizeof(packets::PlayerAcceptPacket), reply.size()));
    }

    packets::PlayerAcceptPacket accept{};
    std::memcpy(&accept, reply.data(), sizeof(accept));
    m_playerId = accept.playerId;
    m_sessionToken = accept.sessionToken;

    std::vector<uint8_t> stateReply;
    if (!tcpframing::readFramed(m_tcpSocket, stateReply, ec)
        || stateReply.size() < sizeof(packets::InitialStatePacket)) {
        throw std::runtime_error("Client did not receive a valid InitialStatePacket: " + ec.message());
    }
    m_bytesReceived += sizeof(packets::TcpMessageHeader) + stateReply.size();

    packets::InitialStatePacket initialState{};
    std::memcpy(&initialState, stateReply.data(), sizeof(initialState));

    const uint8_t* playersDataPtr = stateReply.data() + sizeof(packets::InitialStatePacket);
    size_t expectedSize = sizeof(packets::InitialStatePacket)
                          + (initialState.playerCount * sizeof(packets::OldPlayerInfo));

    if (stateReply.size() == expectedSize) {
        for (uint32_t i = 0; i < initialState.playerCount; i++) {
            packets::OldPlayerInfo oldPlayer{};
            std::memcpy(&oldPlayer, playersDataPtr + (i * sizeof(packets::OldPlayerInfo)), sizeof(oldPlayer));

            m_objectStates[oldPlayer.playerId] = packets::PhysicalObjectState{};
            m_cachedOldPlayers.push_back(oldPlayer);
        }
    }

    std::vector<uint8_t> mapReply;
    if (!tcpframing::readFramed(m_tcpSocket, mapReply, ec) || mapReply.size() < sizeof(packets::InitialMapPacket)) {
        throw std::runtime_error("Client did not receive a valid InitialMapPacket: " + ec.message());
    }
    m_bytesReceived += sizeof(packets::TcpMessageHeader) + mapReply.size();

    packets::InitialMapPacket mapHeader{};
    std::memcpy(&mapHeader, mapReply.data(), sizeof(mapHeader));

    const uint8_t* mapPartsPtr = mapReply.data() + sizeof(packets::InitialMapPacket);
    size_t expectedMapSize
        = sizeof(packets::InitialMapPacket) + (mapHeader.partCount * sizeof(packets::MapPartInfo));

    if (mapReply.size() == expectedMapSize) {
        for (uint32_t i = 0; i < mapHeader.partCount; i++) {
            packets::MapPartInfo mapPart{};
            std::memcpy(&mapPart, mapPartsPtr + (i * sizeof(packets::MapPartInfo)), sizeof(mapPart));

            m_cachedMapParts.push_back(mapPart);
        }
    }

    udp::resolver udpResolver(m_ioContext);
    m_udpServer = *udpResolver.resolve(udp::v4(), host, std::to_string(port)).begin();

    packets::ClientUdpConnectPacket helloPacket{};
    helloPacket.playerId = m_playerId;
    helloPacket.sessionToken = m_sessionToken;

    m_udpSocket.send_to(boost::asio::buffer(&helloPacket, sizeof(helloPacket)), m_udpServer, 0, ec);
    if (ec) {
        throw std::runtime_error("Client failed to send UDP handshake: " + ec.message());
    }
    m_bytesSent += sizeof(helloPacket);

    core::logger::info(std::format("Successfully connected to the server ({}:{})", host, port));
}

Client::~Client() { stop(); }

void Client::sendPhysicsState(const std::vector<packets::PhysicalObjectState>& objects) {
    static constexpr size_t kMaxUdpDatagramSize = 1400;
    constexpr size_t kMaxObjectsPerPacket = (kMaxUdpDatagramSize - sizeof(packets::PhysicsStepPacket))
                                            / sizeof(packets::PhysicalObjectState);

    if (objects.empty())
        return;

    for (size_t offset = 0; offset < objects.size(); offset += kMaxObjectsPerPacket) {
        size_t count = std::min(kMaxObjectsPerPacket, objects.size() - offset);

        packets::PhysicsStepPacket header{};
        header.senderPlayerId = m_playerId;
        header.objectCount = static_cast<uint32_t>(count);

        std::vector<uint8_t> buffer(sizeof(header) + count * sizeof(packets::PhysicalObjectState));
        std::memcpy(buffer.data(), &header, sizeof(header));
        std::memcpy(buffer.data() + sizeof(header), objects.data() + offset, count * sizeof(packets::PhysicalObjectState));

        boost::system::error_code ec;
        m_udpSocket.send_to(boost::asio::buffer(buffer), m_udpServer, 0, ec);
        if (!ec) {
            m_bytesSent += buffer.size();
        }
    }
}

void Client::sendReliable(const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(m_tcpSendMutex);

    boost::system::error_code ec;
    if (!tcpframing::writeFramed(m_tcpSocket, data, size, ec)) {
        core::logger::err("Client sendReliable: " + ec.message());
    } else {
        m_bytesSent += sizeof(packets::TcpMessageHeader) + size;
    }
}

void Client::sendPing() {
    packets::PingPacket ping{};
    ping.clientTimeUs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    boost::system::error_code ec;
    m_udpSocket.send_to(boost::asio::buffer(&ping, sizeof(ping)), m_udpServer, 0, ec);
    if (!ec) {
        m_bytesSent += sizeof(ping);
    }
}

void Client::start() {
    m_running = true;

    if (m_oldPlayersCallback) {
        for (const auto& oldPlayer : m_cachedOldPlayers) {
            m_oldPlayersCallback(oldPlayer.playerId, std::string(oldPlayer.nickname), oldPlayer.position);
        }
    }

    m_cachedOldPlayers.clear();

    if (m_mapCallback) {
        m_mapCallback(m_cachedMapParts);
    }

    m_cachedMapParts.clear();

    m_tcpReceiveThread = std::thread(&Client::tcpUpdateLoop, this);
    m_udpReceiveThread = std::thread(&Client::udpUpdateLoop, this);
    m_statsThread = std::thread(&Client::statsLoop, this);
}

void Client::stop() {
    if (!m_running)
        return;
    m_running = false;

    boost::system::error_code ec;
    if (m_udpSocket.is_open()) {
        m_udpSocket.shutdown(udp::socket::shutdown_both, ec);
        m_udpSocket.close(ec);
    }

    if (m_tcpSocket.is_open()) {
        m_tcpSocket.shutdown(tcp::socket::shutdown_both, ec);
        m_tcpSocket.close(ec);
    }

    if (m_udpReceiveThread.joinable()) {
        m_udpReceiveThread.join();
    }

    if (m_tcpReceiveThread.joinable()) {
        m_tcpReceiveThread.join();
    }

    if (m_statsThread.joinable()) {
        m_statsThread.join();
    }
}

void Client::tcpUpdateLoop() {
    while (m_running) {
        std::vector<uint8_t> payload;
        boost::system::error_code ec;

        if (!tcpframing::readFramed(m_tcpSocket, payload, ec)) {
            if (m_running) {
                core::logger::info("Network client (TCP) receiver stopped: " + ec.message());
            }
            break;
        }

        m_bytesReceived += sizeof(packets::TcpMessageHeader) + payload.size();

        if (payload.size() < sizeof(packets::PacketHeader)) {
            continue;
        }

        auto type = static_cast<packets::PacketType>(payload[0]);

        if (type == packets::PacketType::PlayerJoined && payload.size() == sizeof(packets::PlayerJoinedPacket)) {
            packets::PlayerJoinedPacket joined{};
            std::memcpy(&joined, payload.data(), sizeof(joined));

            m_objectStates[joined.playerId] = packets::PhysicalObjectState{};

            if (m_playerJoinedCallback) {
                m_playerJoinedCallback(joined.playerId, std::string(joined.nickname));
            }
            continue;
        } else if (type == packets::PacketType::PlayerLeft && payload.size() == sizeof(packets::PlayerLeftPacket)) {
            packets::PlayerLeftPacket left{};
            std::memcpy(&left, payload.data(), sizeof(left));

            m_objectStates.erase(left.playerId);

            if (m_playerLeftCallback) {
                m_playerLeftCallback(left.playerId);
            }
            continue;
        } else if (type == packets::PacketType::PlayerKick && payload.size() == sizeof(packets::PlayerKickPacket)) {
            packets::PlayerKickPacket kick{};
            std::memcpy(&kick, payload.data(), sizeof(kick));

            core::logger::err(std::format("Client kicked from server. Reason: {}", kick.reason));
            break;
        }

        if (m_reliableMessageCallback) {
            m_reliableMessageCallback(payload);
        }
    }
}

void Client::udpUpdateLoop() {
    std::vector<uint8_t> buffer(kMaxUdpDatagramSize);

    while (m_running) {
        udp::endpoint sender;
        boost::system::error_code ec;

        size_t bytes = m_udpSocket.receive_from(boost::asio::buffer(buffer), sender, 0, ec);

        if (ec == boost::asio::error::operation_aborted || ec) {
            break;
        }

        m_bytesReceived += bytes;

        if (bytes == sizeof(packets::PongPacket)) {
            packets::PongPacket pong{};
            std::memcpy(&pong, buffer.data(), sizeof(pong));

            if (pong.magic == packets::kPongMagic) {
                auto nowUs = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count());

                if (nowUs >= pong.clientTimeUs) {
                    m_pingMs = static_cast<uint32_t>((nowUs - pong.clientTimeUs) / 1000);
                }
                continue;
            }
        }

        if (bytes < sizeof(packets::PhysicsStepPacket)) {
            continue;
        }

        packets::PhysicsStepPacket header{};
        std::memcpy(&header, buffer.data(), sizeof(header));

        size_t expectedSize
            = sizeof(packets::PhysicsStepPacket)
              + (static_cast<size_t>(header.objectCount) * sizeof(packets::PhysicalObjectState));
        if (bytes != expectedSize) {
            continue;
        }

        const auto* statesPtr
            = reinterpret_cast<const packets::PhysicalObjectState*>(buffer.data() + sizeof(header));

        std::vector<packets::PhysicalObjectState> objects(statesPtr, statesPtr + header.objectCount);
        for (const auto& object : objects) {
            m_objectStates[object.networkId] = object;
        }

        if (m_physicsUpdateCallback) {
            m_physicsUpdateCallback(header.senderPlayerId, objects);
        }
    }

    m_running = false;
}

void Client::statsLoop() {
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

        sendPing();
    }
}

} // namespace net

// NOLINTEND(bugprone-unused-return-value)