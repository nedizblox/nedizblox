#include "game_server.hpp"

#include "utils/utils.hpp"

#include "core/core.hpp"

#include <chrono>
#include <cstring>

namespace game {

GameServer::GameServer(uint16_t port, const std::string& rbxlMapPath) {
    createServer(port);

    createServices();

    buildMap(rbxlMapPath);

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
}

void GameServer::createServer(uint16_t port) {
    m_server = std::make_unique<net::Server>(port);

    m_server->setPhysicsUpdateCallback(
        [this](uint32_t senderPlayerId, const std::vector<net::packets::PhysicalObjectState>& objects) {
            for (const auto& object : objects) {
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
        });
}

void GameServer::buildMap(const std::string& rbxlMapPath) {
    auto instance = utils::rbxl::parseRbxl(rbxlMapPath);

    std::vector<net::packets::MapPartInfo> mapInfos;

    for (auto& instance : instance) {
        if (!instance)
            continue;

        uint32_t networkId = m_nextNetworkId.fetch_add(1);
        instance->setNetworkId(networkId);

        if (!instance->getParent()) {
            instance->setParent(m_workspace);
        }

        if (auto part = std::static_pointer_cast<types::Part>(instance)) {
            m_physics->createRigidBodyPart(part.get());

            {
                std::lock_guard<std::mutex> lock(m_networkPartsMutex);
                m_networkParts[networkId] = part;
            }

            net::packets::MapPartInfo info{};
            info.networkId = networkId;
            info.parentNetworkId = part->getParent()->getNetworkId();
            std::strncpy(info.name, part->getName().c_str(), sizeof(info.name) - 1);
            info.position = part->getPosition();
            info.rotation = part->getOrientation();
            info.size = part->getSize();
            info.color = part->getColor();
            info.transparency = part->getTransparency();
            info.anchored = part->getAnchored();
            info.shape = static_cast<net::packets::PartShape>(part->getShape());

            mapInfos.push_back(info);
        }
    }

    net::packets::InitialMapPacket header{};
    header.partCount = static_cast<uint32_t>(mapInfos.size());

    std::vector<uint8_t> buffer(sizeof(header) + mapInfos.size() * sizeof(net::packets::MapPartInfo));
    std::memcpy(buffer.data(), &header, sizeof(header));
    if (!mapInfos.empty()) {
        std::memcpy(buffer.data() + sizeof(header), mapInfos.data(), mapInfos.size() * sizeof(net::packets::MapPartInfo));
    }

    m_server->setMapData(std::move(buffer));
}

void GameServer::physicsLoop() {
    using clock = std::chrono::steady_clock;

    constexpr float kPhysicsInterval = 1.0f / 60.0f;
    constexpr float kBroadcastInterval = 1.0f / 20.0f;

    auto lastStep = clock::now();
    auto lastBroadcast = lastStep;

    while (m_running) {
        std::this_thread::sleep_for(std::chrono::duration<float>(kPhysicsInterval));

        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - lastStep).count();
        lastStep = now;

        m_physics->step(dt);
        m_physics->stepNetworkInterpolation(dt);

        float sinceBroadcast = std::chrono::duration<float>(now - lastBroadcast).count();
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
                states.push_back(m_physics->captureNetworkState(networkId, body));
            } else if (m_reportedAsleep.insert(networkId).second) {
                states.push_back(m_physics->captureNetworkState(networkId, body));
            }
        }
    }

    if (!states.empty()) {
        m_server->broadcastPhysicsState(states);
    }
}

void GameServer::run() {
    m_physicsThread = std::thread(&GameServer::physicsLoop, this);

    while (!core::sighandler::shouldStop) {
        m_server->update();
    }

    m_running = false;
}

} // namespace game