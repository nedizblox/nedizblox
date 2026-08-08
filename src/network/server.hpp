#pragma once

#include <boost/asio.hpp>

#include "packets.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace net {

class Server {
public:
    using PhysicsUpdateCallback
        = std::function<void(uint32_t senderPlayerId, const std::vector<packets::PhysicalObjectState>& objects)>;

    Server(uint16_t port);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void update();

    void stop();

    void setPhysicsUpdateCallback(PhysicsUpdateCallback callback) {
        m_physicsUpdateCallback = callback;
    }

    void setMapData(std::vector<uint8_t> mapData);

    bool isOwned(uint32_t networkId) const;

    void broadcastPhysicsState(const std::vector<packets::PhysicalObjectState>& objects);

    double getSentKB() const { return static_cast<double>(m_bytesSent.load()) / 1024.0; }
    double getRecvKB() const { return static_cast<double>(m_bytesReceived.load()) / 1024.0; }

    double getSentKBps() const { return m_sentKBps.load(); }
    double getRecvKBps() const { return m_recvKBps.load(); }

private:
    struct TcpClient {
        std::shared_ptr<tcp::socket> socket;
        uint32_t playerId = 0;
        uint64_t sessionToken = 0;
        std::string nickname;
        glm::vec3 lastPosition{0.0f};
    };

    struct ObjectOwnership {
        uint32_t ownerPlayerId = 0;
        std::chrono::steady_clock::time_point lastUpdate;
    };

    static constexpr auto kOwnershipTimeout = std::chrono::seconds(2);
    static constexpr size_t kMaxUdpDatagramSize = 1400;

    boost::asio::io_context m_context;

    udp::socket m_udpSocket;
    std::mutex m_clientsMutex;

    std::unordered_set<udp::endpoint> m_clients;
    std::unordered_map<udp::endpoint, uint32_t> m_udpPlayerIds;

    mutable std::mutex m_objectOwnersMutex;
    std::unordered_map<uint32_t, ObjectOwnership> m_objectOwners;

    std::mutex m_mapDataMutex;
    std::vector<uint8_t> m_mapData;

    tcp::acceptor m_tcpAcceptor;

    std::mutex m_tcpClientsMutex;
    std::unordered_map<uint32_t, TcpClient> m_tcpClients;

    std::atomic<uint32_t> m_nextPlayerId{1};

    std::thread m_acceptThread;
    std::vector<std::thread> m_tcpClientThreads;
    std::thread m_statsThread;
    std::atomic<bool> m_running{true};

    PhysicsUpdateCallback m_physicsUpdateCallback;

    std::atomic<uint64_t> m_bytesSent{0};
    std::atomic<uint64_t> m_bytesReceived{0};
    std::atomic<double> m_sentKBps{0.0};
    std::atomic<double> m_recvKBps{0.0};

    void handleTcpClient(TcpClient client);
    void broadcastReliable(const void* data, size_t size);

    void acceptLoop();
    void statsLoop();

    void kickClient(tcp::socket& socket, const std::string& reason);
    void kickAll(const std::string& reason);
};

} // namespace net