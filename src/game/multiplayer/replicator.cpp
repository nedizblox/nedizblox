#include "replicator.hpp"

namespace game::multiplayer {

Replicator::Replicator(
    const std::string& server, uint16_t port, const std::string& nickname, physics::Physics& physics,
    std::shared_ptr<types::Workspace> workspace) :
    m_physics(physics),  m_workspace(std::move(workspace)) {

    m_client = std::make_unique<net::Client>(server, port, nickname);
    setupCallbacks();
}

Replicator::~Replicator() { stop(); }

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

std::shared_ptr<types::Instance> Replicator::createInstanceByType(enums::InstanceType type) {
    switch (type) {
    case enums::InstanceType::Part:
        return types::Part::create();
    case enums::InstanceType::BillboardText:
        return types::BillboardText::create();
    case enums::InstanceType::Decal:
        return types::Decal::create();
    case enums::InstanceType::SelectionBox:
        return types::SelectionBox::create();
    case enums::InstanceType::Model:
        return types::Model::create();
    case enums::InstanceType::Folder:
        return types::Folder::create();
    default:
        return nullptr;
    }
}

void Replicator::setupCallbacks() {
    m_client->setMapCallback([this](const std::vector<uint8_t>& mapBuffer) {
        net::serializer::BinaryReader reader(mapBuffer.data(), mapBuffer.size());

        uint32_t objectCount = reader.read<uint32_t>();

        std::vector<PendingParenting> pendingList;
        pendingList.reserve(objectCount);

        std::unordered_map<uint32_t, std::shared_ptr<types::Instance>> instancesById;
        instancesById.reserve(objectCount);

        for (uint32_t i = 0; i < objectCount; ++i) {
            auto type = reader.read<enums::InstanceType>();

            auto instance = createInstanceByType(type);
            if (!instance) {
                continue;
            }

            instance->deserialize(reader);

            uint32_t netId = instance->getNetworkId();
            uint32_t parentId = instance->getParentId();

            m_networkInstances[netId] = instance;
            instancesById[instance->getId()] = instance;

            if (instance->getType() == enums::InstanceType::Part) {
                auto part = std::static_pointer_cast<types::Part>(instance);

                btRigidBody* rigidBody = m_physics.createRigidBodyPart(part.get());
                m_physics.setBodyNetworkMode(rigidBody, false);
                part->setRigidBody(rigidBody);

                m_networkInstances[netId] = part;
                instancesById[instance->getId()] = part;
            }

            pendingList.push_back({instance, parentId});
        }

        for (const auto& item : pendingList) {
            if (!item.instance)
                continue;

            if (item.parentId == 0) {
                item.instance->setParent(m_workspace);
                continue;
            }

            auto parentIt = instancesById.find(item.parentId);
            if (parentIt != instancesById.end() && parentIt->second) {
                item.instance->setParent(parentIt->second);
            } else {
                item.instance->setParent(m_workspace);
            }
        }
    });

    m_client->setOldPlayersCallback([this](uint32_t playerId, const std::string& nickname) {
        if (playerId == m_client->getPlayerId())
            return;

        auto rig = prefabs::Rig::create(m_physics, nickname, playerId);
        rig->setParent(m_workspace);
        // rig->setPivotPosition(position);

        m_physics.setBodyNetworkMode(rig->getRigidBody(), false);
        m_networkRigs[playerId] = std::move(rig);
    });

    m_client->setPlayerJoinedCallback([this](uint32_t playerId, const std::string& nickname) {
        if (playerId == m_client->getPlayerId())
            return;

        auto rig = prefabs::Rig::create(m_physics, nickname, playerId);
        rig->setParent(m_workspace);

        m_physics.setBodyNetworkMode(rig->getRigidBody(), false);
        m_networkRigs[playerId] = std::move(rig);
    });

    m_client->setPlayerLeftCallback([this](uint32_t playerId) {
        auto it = m_networkRigs.find(playerId);

        if (it != m_networkRigs.end() && it->second) {
            auto& rig = it->second;
            rig->destroy();
            m_networkRigs.erase(playerId);
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
                    auto instanceIt = m_networkInstances.find(object.networkId);
                    instanceIt != m_networkInstances.end() && instanceIt->second
                    && instanceIt->second->getType() == enums::InstanceType::Part) {
                    auto part = std::static_pointer_cast<types::Part>(instanceIt->second);
                    body = part->getRigidBody();
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
