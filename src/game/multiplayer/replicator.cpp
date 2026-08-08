#include "replicator.hpp"

namespace game::multiplayer {

Replicator::Replicator(
    const std::string& server, uint16_t port, const std::string& nickname, physics::Physics& physics,
    mngrs::InstanceManager& instanceManager, std::shared_ptr<types::Workspace> workspace) :
    m_physics(physics), m_instanceManager(instanceManager), m_workspace(std::move(workspace)) {

    m_client = std::make_unique<net::Client>(server, port, nickname);
    setupCallbacks();
}

Replicator::~Replicator() {
    stop();
    m_networkParts.clear();
    m_networkRigs.clear();
}

void Replicator::start() { m_client->start(); }

void Replicator::stop() {
    if (m_client) {
        m_client->stop();
    }
}

void Replicator::sendPhysicsState(const std::vector<net::packets::PhysicalObjectState>& states) {
    if (!states.empty()) {
        m_client->sendPhysicsState(states);
    }
}

void Replicator::updateNetworkRigs(float dt) {
    for (auto& [id, rig] : m_networkRigs) {
        if (rig) {
            rig->update(dt);
        }
    }
}

void Replicator::setupCallbacks() {
    m_client->setMapCallback([this](const std::vector<net::packets::MapPartInfo>& parts) {
        for (const auto& info : parts) {
            auto part = std::make_shared<types::Part>();
            part->setNetworkId(info.networkId);

            std::string name(info.name);
            if (!name.empty())
                part->setName(name);

            part->setPosition(info.position);
            part->setOrientation(info.rotation);
            part->setSize(info.size);
            part->setColor(info.color);
            part->setTransparency(info.transparency);
            part->setAnchored(info.anchored);
            part->setShape(static_cast<enums::PartType>(info.shape));

            btRigidBody* rigidBody = m_physics.createRigidBodyPart(part.get());
            m_physics.setBodyNetworkMode(rigidBody, false);

            part->setRigidBody(rigidBody);

            m_networkParts[info.networkId] = std::move(part);
        }

        for (const auto& info : parts) {
            auto it = m_networkParts.find(info.networkId);
            if (it == m_networkParts.end() || !it->second)
                continue;

            if (info.parentNetworkId == 0) {
                it->second->setParent(m_workspace);
                continue;
            }

            auto parentIt = m_networkParts.find(info.parentNetworkId);
            if (parentIt != m_networkParts.end() && parentIt->second) {
                it->second->setParent(parentIt->second);
            } else {
                it->second->setParent(m_workspace);
            }
        }

        m_instanceManager.markMapDirty();
    });

    m_client->setOldPlayersCallback([this](uint32_t playerId, const std::string& nickname, const glm::vec3& position) {
        if (playerId == m_client->getPlayerId())
            return;

        auto rig = prefabs::Rig::create(m_physics, nickname, playerId);
        rig->setParent(m_workspace);
        rig->setPivotPosition(position);

        m_physics.setBodyNetworkMode(rig->getRigidBody(), false);
        m_networkRigs[playerId] = std::move(rig);
        m_instanceManager.markMapDirty();
    });

    m_client->setPlayerJoinedCallback([this](uint32_t playerId, const std::string& nickname) {
        if (playerId == m_client->getPlayerId())
            return;

        auto rig = prefabs::Rig::create(m_physics, nickname, playerId);
        rig->setParent(m_workspace);

        m_physics.setBodyNetworkMode(rig->getRigidBody(), false);
        m_networkRigs[playerId] = std::move(rig);

        m_instanceManager.markMapDirty();
    });

    m_client->setPlayerLeftCallback([this](uint32_t playerId) {
        auto it = m_networkRigs.find(playerId);

        if (it != m_networkRigs.end() && it->second) {
            auto& rig = it->second;
            rig->destroy();
            m_networkRigs.erase(playerId);

            m_instanceManager.markMapDirty();
        }
    });

    m_client->setPhysicsUpdateCallback(
        [this](uint32_t senderPlayerId, const std::vector<net::packets::PhysicalObjectState>& objects) {
            for (const auto& object : objects) {
                btRigidBody* body = nullptr;

                if (auto rigIt = m_networkRigs.find(object.networkId);
                    rigIt != m_networkRigs.end() && rigIt->second) {
                    body = rigIt->second->getRigidBody();
                } else if (
                    auto partIt = m_networkParts.find(object.networkId);
                    partIt != m_networkParts.end() && partIt->second) {
                    body = partIt->second->getRigidBody();
                }

                if (body) {
                    bool isKinematic
                        = (body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) != 0;
                    if (isKinematic) {
                        m_physics.applyNetworkState(object);
                    }
                }
            }
        });
}

} // namespace game::multiplayer