#include "client_replicator.hpp"

#include "../types/factory.hpp"

namespace game::multiplayer {

ClientReplicator::ClientReplicator(
    const std::string& server, uint16_t port, const std::string& nickname,
    physics::Physics& physics, mngrs::HumanoidManager& humanoids,
    std::shared_ptr<types::Workspace> workspace
) :
    m_physics(physics), m_humanoids(humanoids),
    m_workspace(std::move(workspace)) {

    if (!server.empty() || port != 0) {
        m_client = std::make_unique<net::Client>(server, port, nickname);
        setupCallbacks();
    } else {
        m_offlineMode = true;
    }
}

ClientReplicator::~ClientReplicator() {
    stop();
}

void ClientReplicator::start() {
    if (!m_offlineMode)
        m_client->start();
}

void ClientReplicator::stop() {
    if (m_client && !m_offlineMode)
        m_client->stop();
}

void ClientReplicator::sendPhysicsState(
    const std::vector<net::packets::PhysicalObjectState>& states
) {
    if (!states.empty() && !m_offlineMode) {
        m_client->sendPhysicsState(states);
    }
}

void ClientReplicator::addRemotePlayer(
    uint32_t playerId, const std::string& nickname
) {
    if (playerId == m_client->getPlayerId())
        return;

    auto rig = m_humanoids.createHumanoid(nickname, playerId, false);
    if (rig)
        m_networkRigs[playerId] = std::move(rig);
}

void ClientReplicator::setupCallbacks() {
    m_client->setMapCallback(
        [this](const std::vector<uint8_t>& mapBuffer)
        {
            net::serializer::BinaryReader reader(
                mapBuffer.data(), mapBuffer.size()
            );

            uint32_t objectCount = reader.read<uint32_t>();

            std::vector<PendingParenting> pendingList;
            pendingList.reserve(objectCount);

            std::unordered_map<uint32_t, std::shared_ptr<types::Instance>>
                instancesById;
            instancesById.reserve(objectCount);

            for (uint32_t i = 0; i < objectCount; ++i) {
                auto type = reader.read<enums::InstanceType>();

                auto instance = types::factory::create(type);
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

                    btRigidBody* rigidBody =
                        m_physics.createRigidBodyPart(part.get());
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
        }
    );

    m_client->setOldPlayersCallback(
        [this](uint32_t playerId, const std::string& nickname)
        { addRemotePlayer(playerId, nickname); }
    );

    m_client->setPlayerJoinedCallback(
        [this](uint32_t playerId, const std::string& nickname)
        { addRemotePlayer(playerId, nickname); }
    );

    m_client->setPlayerLeftCallback(
        [this](uint32_t playerId)
        {
            m_humanoids.removeHumanoid(playerId);
            m_networkRigs.erase(playerId);
        }
    );

    m_client->setPhysicsUpdateCallback(
        [this](
            uint32_t senderPlayerId,
            const std::vector<net::packets::PhysicalObjectState>& objects
        )
        {
            for (const auto& object : objects) {
                btRigidBody* body = nullptr;

                if (auto rigIt = m_networkRigs.find(object.networkId);
                    rigIt != m_networkRigs.end() && rigIt->second) {
                    body = rigIt->second->getRigidBody();
                } else if (auto instanceIt =
                               m_networkInstances.find(object.networkId);
                           instanceIt != m_networkInstances.end() &&
                           instanceIt->second &&
                           instanceIt->second->getType() ==
                               enums::InstanceType::Part) {
                    auto part = std::static_pointer_cast<types::Part>(
                        instanceIt->second
                    );
                    body = part->getRigidBody();
                }

                if (body) {
                    bool isKinematic =
                        (body->getCollisionFlags() &
                         btCollisionObject::CF_KINEMATIC_OBJECT) != 0;
                    if (isKinematic) {
                        m_physics.applyNetworkState(object);
                    }
                }
            }
        }
    );
}

} // namespace game::multiplayer