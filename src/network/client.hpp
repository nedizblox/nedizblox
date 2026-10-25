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
    using OldPlayersCallback =
        std::function<void(uint32_t, const std::string&)>;
    using PlayerJoinedCallback =
        std::function<void(uint32_t, const std::string&)>;
    using PlayerLeftCallback = std::function<void(uint32_t)>;

    using NetworkMessageCallback = std::function<void(const std::string&)>;

    using PhysicsUpdateCallback = std::function<
        void(uint32_t, const std::vector<packets::PhysicalObjectState>&)>;

    using MapCallback = std::function<void(const std::vector<uint8_t>&)>;

    using ReliableMessageCallback =
        std::function<void(const std::vector<uint8_t>&)>;

    Client(const std::string& host, uint16_t port, const std::string& nickname);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void setOldPlayersCallback(OldPlayersCallback callback) {
        m_oldPlayersCallback = callback;
    }
    void setPlayerJoinedCallback(PlayerJoinedCallback callback) {
        m_playerJoinedCallback = callback;
    }
    void setPlayerLeftCallback(PlayerLeftCallback callback) {
        m_playerLeftCallback = callback;
    }

    void setNetworkMessageCallback(NetworkMessageCallback callback) {
        m_networkMessageCallback = callback;
    }

    void setPhysicsUpdateCallback(PhysicsUpdateCallback callback) {
        m_physicsUpdateCallback = callback;
    }
    void setMapCallback(MapCallback callback) {
        m_mapCallback = callback;
    }

    void setReliableMessageCallback(ReliableMessageCallback callback) {
        m_reliableMessageCallback = callback;
    }

    std::string getServerAddress() const {
        return m_serverAddress;
    }
    uint16_t getServerPort() const {
        return m_serverPort;
    }

    uint32_t getPlayerId() const {
        return m_playerId;
    }
    std::string getPlayerNickname() const {
        return m_playerNickname;
    }

    void
    sendPhysicsState(const std::vector<packets::PhysicalObjectState>& objects);

    void sendReliable(const void* data, size_t size);

    void sendPing();

    uint32_t getPingMs() const {
        return m_pingMs.load();
    }

    double getSentKB() const {
        return static_cast<double>(m_bytesSent.load()) / 1024.0;
    }
    double getRecvKB() const {
        return static_cast<double>(m_bytesReceived.load()) / 1024.0;
    }

    double getSentKBps() const {
        return m_sentKBps.load();
    }
    double getRecvKBps() const {
        return m_recvKBps.load();
    }

    void start();
    void stop();

private:
    static constexpr size_t kMaxUdpDatagramSize = 1400;

    boost::asio::io_context m_ioContext;

    std::string m_serverAddress = "";
    uint16_t m_serverPort = 0;
    std::string m_playerNickname = "";

    tcp::socket m_tcpSocket;
    std::mutex m_tcpSendMutex;

    udp::socket m_udpSocket;
    udp::endpoint m_udpServer;

    uint32_t m_playerId = 0;
    uint64_t m_sessionToken = 0;

    std::thread m_tcpReceiveThread;
    std::thread m_udpReceiveThread;
    std::thread m_statsThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_udpConfirmed{
        false
    };

    std::unordered_map<uint32_t, packets::PhysicalObjectState> m_objectStates;
    std::vector<packets::OldPlayerInfo> m_cachedOldPlayers;
    std::vector<uint8_t> m_cachedMapBuffer;

    OldPlayersCallback m_oldPlayersCallback;
    PlayerJoinedCallback m_playerJoinedCallback;
    PlayerLeftCallback m_playerLeftCallback;
    NetworkMessageCallback m_networkMessageCallback;
    PhysicsUpdateCallback m_physicsUpdateCallback;
    MapCallback m_mapCallback;

    ReliableMessageCallback m_reliableMessageCallback;

    std::atomic<uint32_t> m_pingMs{0};
    std::atomic<uint64_t> m_bytesSent{0};
    std::atomic<uint64_t> m_bytesReceived{0};
    std::atomic<double> m_sentKBps{0.0};
    std::atomic<double> m_recvKBps{0.0};

    bool sendUdpHello();

    void tcpUpdateLoop();
    void udpUpdateLoop();
    void statsLoop();
};

} // namespace net