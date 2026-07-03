#include "client.hpp"

#include "tcp_framing.hpp"

#include "core/logger.hpp"

#include <cstring>
#include <stdexcept>

// NOLINTBEGIN(bugprone-unused-return-value)

namespace net {

Client::Client(
    boost::asio::io_context& context, const std::string& host, uint16_t tcpPort, uint16_t udpPort,
    const std::string& nickname) :
    m_udpSocket(context, udp::v4()), m_tcpSocket(context) {
    tcp::resolver tcpResolver(context);
    auto tcpEndpoints = tcpResolver.resolve(host, std::to_string(tcpPort));

    boost::system::error_code ec;
    boost::asio::connect(m_tcpSocket, tcpEndpoints, ec);

    if (ec) {
        throw std::runtime_error("Client TCP connect failed: " + ec.message());
    }

    packets::PlayerInfoPacket info{};
    std::strncpy(info.nickname, nickname.c_str(), sizeof(info.nickname) - 1);

    if (!tcpframing::writeFramed(m_tcpSocket, &info, sizeof(info), ec)) {
        throw std::runtime_error("Client failed to send PlayerInfoPacket: " + ec.message());
    }

    std::vector<uint8_t> reply;
    if (!tcpframing::readFramed(m_tcpSocket, reply, ec)) {
        throw std::runtime_error("Client did not receive TCP handshake response: " + ec.message());
    }

    if (reply.size() == sizeof(packets::PlayerKickPacket)) {
        packets::PlayerKickPacket kick{};
        std::memcpy(&kick, reply.data(), sizeof(kick));
        throw std::runtime_error(std::format("Client kicked from server. Reason: {}", kick.reason));
    }

    if (reply.size() != sizeof(packets::PlayerAcceptPacket)) {
        throw std::runtime_error("Client did not receive a valid PlayerAcceptPacket: Size mismatch");
    }

    packets::PlayerAcceptPacket accept{};
    std::memcpy(&accept, reply.data(), sizeof(accept));
    m_playerId = accept.playerId;

    std::vector<uint8_t> stateReply;
    if (!tcpframing::readFramed(m_tcpSocket, stateReply, ec)
        || stateReply.size() < sizeof(packets::InitialStatePacket)) {
        throw std::runtime_error("Client did not receive a valid InitialStatePacket: " + ec.message());
    }

    packets::InitialStatePacket initialState{};
    std::memcpy(&initialState, stateReply.data(), sizeof(initialState));

    const uint8_t* playersDataPtr = stateReply.data() + sizeof(packets::InitialStatePacket);
    size_t expectedSize = sizeof(packets::InitialStatePacket)
                          + (initialState.playerCount * sizeof(packets::OldPlayerInfo));

    if (stateReply.size() == expectedSize) {
        for (uint32_t i = 0; i < initialState.playerCount; ++i) {
            packets::OldPlayerInfo oldPlayer{};
            std::memcpy(&oldPlayer, playersDataPtr + (i * sizeof(packets::OldPlayerInfo)), sizeof(oldPlayer));

            m_players[oldPlayer.playerId] = packets::RigMoveBroadcastPacket{};

            m_cachedOldPlayers.push_back(oldPlayer);
        }
    }

    core::logger::info("Client TCP handshake complete");

    udp::resolver udpResolver(context);
    m_udpServer = *udpResolver.resolve(udp::v4(), host, std::to_string(udpPort)).begin();
}

Client::~Client() {
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
}

void Client::sendMovement(packets::RigMovePacket packet) {
    packet.playerId = m_playerId;

    boost::system::error_code ec;
    m_udpSocket.send_to(boost::asio::buffer(&packet, sizeof(packet)), m_udpServer, 0, ec);

    if (ec) {
        core::logger::err("Client send: " + ec.message());
    }
}

void Client::sendReliable(const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(m_tcpSendMutex);

    boost::system::error_code ec;
    if (!tcpframing::writeFramed(m_tcpSocket, data, size, ec)) {
        core::logger::err("Client sendReliable: " + ec.message());
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

    packets::ClientUdpConnectPacket helloPacket{};
    helloPacket.playerId = m_playerId;

    boost::system::error_code ec;
    m_udpSocket.send_to(boost::asio::buffer(&helloPacket, sizeof(helloPacket)), m_udpServer, 0, ec);
    if (ec) {
        core::logger::err("Client failed to send UDP handshake: " + ec.message());
    } else {
        core::logger::info("Client UDP handshake complete");
    }

    m_tcpReceiveThread = std::thread(&Client::tcpUpdateLoop, this);
    m_udpReceiveThread = std::thread(&Client::udpUpdateLoop, this);
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

        if (payload.size() == sizeof(packets::PlayerJoinedPacket)) {
            packets::PlayerJoinedPacket joined{};
            std::memcpy(&joined, payload.data(), sizeof(joined));

            m_players[joined.playerId] = packets::RigMoveBroadcastPacket{};

            if (m_playerJoinedCallback) {
                m_playerJoinedCallback(joined.playerId, std::string(joined.nickname));
            }
            continue;
        } else if (payload.size() == sizeof(packets::PlayerLeftPacket)) {
            packets::PlayerLeftPacket left{};
            std::memcpy(&left, payload.data(), sizeof(left));

            m_players.erase(left.playerId);

            if (m_playerLeftCallback) {
                m_playerLeftCallback(left.playerId);
            }
            continue;
        } else if (payload.size() == sizeof(packets::PlayerKickPacket)) {
            packets::PlayerKickPacket kick{};
            std::memcpy(&kick, payload.data(), sizeof(kick));

            core::logger::err(std::format("Client kicked from server. Reason: {}", kick.reason));

            m_running = false;
            break;
        }

        if (m_reliableMessageCallback) {
            m_reliableMessageCallback(payload);
        }
    }
}

void Client::udpUpdateLoop() {
    while (m_running) {
        packets::RigMoveBroadcastPacket packet{};
        udp::endpoint sender;
        boost::system::error_code ec;

        size_t bytes = m_udpSocket.receive_from(
            boost::asio::buffer(&packet, sizeof(packets::RigMoveBroadcastPacket)), sender, 0, ec);

        if (ec == boost::asio::error::operation_aborted || ec) {
            break;
        }

        if (bytes == sizeof(packets::RigMoveBroadcastPacket)) {
            auto it = m_players.find(packet.playerId);

            if (it != m_players.end()) {
                it->second = packet;
                if (m_playerUpdateCallback) {
                    m_playerUpdateCallback(packet);
                }
            }
        }
    }
}

} // namespace net

// NOLINTEND(bugprone-unused-return-value)