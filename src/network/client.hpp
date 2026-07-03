#pragma once

#include <boost/asio.hpp>

#include "packets.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace net {

class Client {
public:
    using OldPlayersCallback = std::function<void(uint32_t, const std::string&, const glm::vec3&)>;
    using PlayerJoinedCallback = std::function<void(uint32_t, const std::string&)>;
    using PlayerLeftCallback = std::function<void(uint32_t)>;
    using PlayerUpdateCallback = std::function<void(const packets::RigMoveBroadcastPacket&)>;

    using ReliableMessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    Client(boost::asio::io_context& context, const std::string& host, uint16_t tcpPort, uint16_t udpPort, const std::string& nickname);
    ~Client();

    void setOldPlayersCallback(OldPlayersCallback callback) { m_oldPlayersCallback = callback; }

    void setPlayerJoinedCallback(PlayerJoinedCallback callback) {
        m_playerJoinedCallback = callback;
    }

    void setPlayerLeftCallback(PlayerLeftCallback callback) { m_playerLeftCallback = callback; }

    void setPlayerUpdateCallback(PlayerUpdateCallback callback) {
        m_playerUpdateCallback = callback;
    }

    void setReliableMessageCallback(ReliableMessageCallback callback) {
        m_reliableMessageCallback = callback;
    }

    uint32_t getPlayerId() const { return m_playerId; }
    void sendMovement(packets::RigMovePacket packet);
    void sendReliable(const void* data, size_t size);
    void start();

private:
    tcp::socket m_tcpSocket;
    std::mutex m_tcpSendMutex;

    udp::socket m_udpSocket;
    udp::endpoint m_udpServer;

    uint32_t m_playerId = 0;

    std::thread m_tcpReceiveThread;
    std::thread m_udpReceiveThread;
    std::atomic<bool> m_running{false};

    std::unordered_map<uint32_t, packets::RigMoveBroadcastPacket> m_players;
    std::vector<packets::OldPlayerInfo> m_cachedOldPlayers;

    OldPlayersCallback m_oldPlayersCallback;
    PlayerJoinedCallback m_playerJoinedCallback;
    PlayerLeftCallback m_playerLeftCallback;
    PlayerUpdateCallback m_playerUpdateCallback;

    ReliableMessageCallback m_reliableMessageCallback;

    void tcpUpdateLoop();
    void udpUpdateLoop();
};

} // namespace net