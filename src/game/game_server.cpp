#include "game_server.hpp"

#include "utils/utils.hpp"

#include "core/core.hpp"

#include <chrono>
#include <cstring>

namespace game {

GameServer::GameServer(uint16_t port, const std::string& nbxlPath) {
    createServices();

    createServer(port);

    buildMap(nbxlPath);

    core::sighandler::listen();
}

GameServer::~GameServer() {
    m_running = false;

    if (m_physicsThread.joinable()) {
        m_physicsThread.join();
    }
}

void GameServer::createServices() {
    m_workspace = std::make_shared<types::Workspace>();

    // m_coreGui = std::make_shared<types::CoreGui>();

    m_physics = std::make_unique<physics::Physics>(m_workspace);

    m_humanoidManager =
        std::make_unique<mngrs::HumanoidManager>(*m_physics, m_workspace);
}

void GameServer::createServer(uint16_t port) {
    m_server = std::make_unique<net::Server>(port);

    m_server->setPlayerJoinedCallback(
        [this](uint32_t playerId, const std::string& nickname)
        {
            std::lock_guard<std::mutex> lock(m_playerEventsMutex);
            m_playerEvents.push_back({true, playerId, nickname});
        }
    );

    m_server->setPlayerLeftCallback(
        [this](uint32_t playerId)
        {
            std::lock_guard<std::mutex> lock(m_playerEventsMutex);
            m_playerEvents.push_back({false, playerId, {}});
        }
    );

    m_server->setPhysicsUpdateCallback(
        [this](
            uint32_t senderPlayerId,
            const std::vector<net::packets::PhysicalObjectState>& objects
        )
        {
            for (const auto& object : objects) {
                if (m_humanoidManager->hasHumanoid(object.networkId)) {
                    if (object.networkId == senderPlayerId)
                        m_physics->applyNetworkState(object);
                    continue;
                }

                std::shared_ptr<types::Part> part = nullptr;

                {
                    std::lock_guard<std::mutex> lock(m_networkPartsMutex);
                    auto it = m_networkParts.find(object.networkId);
                    if (it != m_networkParts.end()) {
                        part = it->second;
                    }
                }

                if (!part)
                    continue;

                btRigidBody* body = part->getRigidBody();
                if (body) {
                    m_physics->setBodyNetworkMode(body, false);
                    m_physics->applyNetworkState(object);
                }
            }
        }
    );
}

void GameServer::buildMap(const std::string& nbxlPath) {
    auto parsedRoot = utils::nbxf::parseNbxl(nbxlPath);

    for (auto& inst : parsedRoot) {
        if (inst && !inst->getParent()) {
            inst->setParent(m_workspace);
        }
    }

    m_replicator = std::make_unique<multiplayer::ServerReplicator>(*m_physics);
    m_replicator->setRoot(m_workspace);
    m_replicator->rebuildMap();

    syncNetworkParts();

    m_server->setMapData(m_replicator->getMapBuffer());
}

void GameServer::physicsLoop() {
    constexpr float kPhysicsInterval = 1.0f / 60.0f;
    constexpr float kBroadcastInterval = 1.0f / 20.0f;

    auto lastStep = std::chrono::steady_clock::now();
    auto lastBroadcast = lastStep;

    while (m_running) {
        std::this_thread::sleep_for(
            std::chrono::duration<float>(kPhysicsInterval)
        );

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastStep).count();
        lastStep = now;

        m_physics->update(dt);
        m_humanoidManager->update(dt);

        float sinceBroadcast =
            std::chrono::duration<float>(now - lastBroadcast).count();
        if (sinceBroadcast >= kBroadcastInterval) {
            lastBroadcast = now;
            broadcastUnownedObjects();
        }
    }
}

void GameServer::broadcastUnownedObjects() {
    std::vector<net::packets::PhysicalObjectState> states;

    {
        std::lock_guard<std::mutex> lock(m_networkPartsMutex);
        states.reserve(m_networkParts.size());

        for (const auto& [networkId, part] : m_networkParts) {
            if (!part || part->getAnchored())
                continue;

            btRigidBody* body = part->getRigidBody();
            if (!body)
                continue;

            bool owned = m_server->isOwned(networkId);

            m_physics->setBodyNetworkMode(body, !owned);

            if (owned) {
                m_reportedAsleep.erase(networkId);
                continue;
            }

            if (body->isActive()) {
                m_reportedAsleep.erase(networkId);
                states.push_back(
                    m_physics->captureNetworkState(networkId, body)
                );
            } else if (m_reportedAsleep.insert(networkId).second) {
                states.push_back(
                    m_physics->captureNetworkState(networkId, body)
                );
            }
        }
    }

    if (!states.empty()) {
        m_server->broadcastPhysicsState(states);
    }
}

void GameServer::syncNetworkParts() {
    auto parts = m_replicator->getNetworkParts();

    std::lock_guard<std::mutex> lock(m_networkPartsMutex);
    m_networkParts = std::move(parts);
}

void GameServer::processPlayerEvents() {
    std::vector<PlayerEvent> events;

    {
        std::lock_guard<std::mutex> lock(m_playerEventsMutex);
        events.swap(m_playerEvents);
    }

    for (const auto& event : events) {
        if (event.joined) {
            m_humanoidManager->createHumanoid(
                event.nickname, event.playerId, false
            );
        } else {
            m_humanoidManager->removeHumanoid(event.playerId);
        }
    }
}

void GameServer::run() {
    m_physics->start();

    m_physicsThread = std::thread(&GameServer::physicsLoop, this);

    while (!core::sighandler::shouldStop) {
        processPlayerEvents();

        if (m_replicator->isDirty()) {
            m_replicator->rebuildMap();
            syncNetworkParts();
            m_server->setMapData(m_replicator->getMapBuffer());
        }

        m_server->update();
    }

    m_running = false;
}

} // namespace game