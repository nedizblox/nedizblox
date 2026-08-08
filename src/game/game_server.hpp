#pragma once

#include "network/network.hpp"

#include "types/types.hpp"

#include "physics/physics.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace game {

class GameServer {
public:
    GameServer(uint16_t port, const std::string& rbxlMapPath);
    ~GameServer();

    GameServer(const GameServer&) = delete;
    GameServer& operator=(const GameServer&) = delete;

    void run();

private:
    std::unique_ptr<net::Server> m_server;

    std::unique_ptr<physics::Physics> m_physics;

    std::shared_ptr<types::Workspace> m_workspace;
    // std::shared_ptr<types::CoreGui> m_coreGui;

    std::mutex m_networkPartsMutex;
    std::unordered_map<uint32_t, std::shared_ptr<types::Part>> m_networkParts;
    std::atomic<uint32_t> m_nextNetworkId{1000};

    std::unordered_set<uint32_t> m_reportedAsleep;

    std::thread m_physicsThread;

    std::atomic<bool> m_running{true};

    void createServices();

    void createServer(uint16_t port);

    void buildMap(const std::string& rbxlMapPath);

    void physicsLoop();
    void broadcastUnownedObjects();
};

} // namespace game