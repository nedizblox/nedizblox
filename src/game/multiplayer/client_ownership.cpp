#include "client_ownership.hpp"

#include <algorithm>

namespace game::multiplayer {

ClientOwnership::ClientOwnership(physics::Physics& physics, uint32_t playerId) :
    m_physics(physics), m_playerId(playerId) {
}

std::vector<net::packets::PhysicalObjectState> ClientOwnership::update(
    float dt, const std::shared_ptr<types::Rig>& localRig,
    const std::unordered_map<uint32_t, std::shared_ptr<types::Instance>>&
        networkInstances
) {

    std::vector<net::packets::PhysicalObjectState> outgoingStates;

    if (!localRig)
        return outgoingStates;

    btRigidBody* playerBody = localRig->getRigidBody();
    if (!playerBody)
        return outgoingStates;

    auto getPart =
        [&networkInstances](uint32_t netId) -> std::shared_ptr<types::Part>
    {
        auto it = networkInstances.find(netId);
        if (it != networkInstances.end() && it->second &&
            it->second->getType() == enums::InstanceType::Part) {
            return std::static_pointer_cast<types::Part>(it->second);
        }
        return nullptr;
    };

    std::unordered_set<uint32_t> candidates;
    std::vector<SearchNode> frontier;

    std::vector<uint32_t> touched = m_physics.getBodyCollisions(playerBody);
    for (uint32_t netId : touched) {
        frontier.push_back({netId, 0});
    }

    std::unordered_set<uint32_t> visited;

    while (!frontier.empty()) {
        SearchNode current = frontier.back();
        frontier.pop_back();

        if (!visited.insert(current.netId).second)
            continue;

        auto part = getPart(current.netId);
        if (!part || part->getAnchored())
            continue;

        candidates.insert(current.netId);

        if (current.depth < kMaxCollisionDepth) {
            btRigidBody* b = part->getRigidBody();
            if (b) {
                std::vector<uint32_t> more = m_physics.getBodyCollisions(b);
                for (uint32_t nextId : more) {
                    if (visited.find(nextId) == visited.end()) {
                        frontier.push_back({nextId, current.depth + 1});
                    }
                }
            }
        }
    }

    for (uint32_t netId : candidates) {
        auto part = getPart(netId);
        btRigidBody* b = part ? part->getRigidBody() : nullptr;

        if (!part || !b) {
            auto staleIt = std::find(
                m_ownershipOrder.begin(), m_ownershipOrder.end(), netId
            );
            if (staleIt != m_ownershipOrder.end())
                m_ownershipOrder.erase(staleIt);
            m_reportedAsleepIds.erase(netId);
            continue;
        }

        auto orderIt =
            std::find(m_ownershipOrder.begin(), m_ownershipOrder.end(), netId);

        if (orderIt == m_ownershipOrder.end()) {
            if (m_ownershipOrder.size() >= kMaxLocallyOwnedObjects) {
                auto evictIt = std::find_if(
                    m_ownershipOrder.begin(), m_ownershipOrder.end(),
                    [&](uint32_t candidateId)
                    {
                        auto candidatePart = getPart(candidateId);
                        btRigidBody* candidateBody =
                            candidatePart ? candidatePart->getRigidBody()
                                          : nullptr;
                        return candidateBody && !candidateBody->isActive();
                    }
                );

                if (evictIt != m_ownershipOrder.end()) {
                    uint32_t evictedId = *evictIt;
                    m_ownershipOrder.erase(evictIt);
                    m_reportedAsleepIds.erase(evictedId);

                    auto evictedPart = getPart(evictedId);
                    if (evictedPart) {
                        if (btRigidBody* evictedBody =
                                evictedPart->getRigidBody()) {
                            m_physics.setBodyNetworkMode(evictedBody, false);
                        }
                    }
                }
            }

            m_physics.setBodyNetworkMode(b, true);
            m_ownershipOrder.push_back(netId);
        } else {
            m_ownershipOrder.erase(orderIt);
            m_ownershipOrder.push_back(netId);
        }
    }

    m_netSendAccumulator += dt;
    if (m_netSendAccumulator < kNetSendInterval) {
        return outgoingStates;
    }

    m_netSendAccumulator -= kNetSendInterval;

    std::vector<uint32_t> activeOwnerships(
        m_ownershipOrder.begin(), m_ownershipOrder.end()
    );

    for (uint32_t netId : activeOwnerships) {
        auto part = getPart(netId);
        btRigidBody* body = part ? part->getRigidBody() : nullptr;

        if (!part || !body) {
            auto orderIt = std::find(
                m_ownershipOrder.begin(), m_ownershipOrder.end(), netId
            );
            if (orderIt != m_ownershipOrder.end())
                m_ownershipOrder.erase(orderIt);
            m_reportedAsleepIds.erase(netId);
            continue;
        }

        bool isFarAway = false;
        btVector3 btPos = body->getWorldTransform().getOrigin();
        float distSq = glm::distance2(
            localRig->getPivotPosition(),
            glm::vec3(btPos.x(), btPos.y(), btPos.z())
        );

        if (distSq > kReleaseDistanceSq) {
            isFarAway = true;
        }

        bool isContactLost = (candidates.find(netId) == candidates.end());
        bool shouldRelease = isFarAway || (isContactLost && !body->isActive());

        if (shouldRelease) {
            outgoingStates.push_back(
                m_physics.captureNetworkState(netId, body)
            );
            m_physics.setBodyNetworkMode(body, false);
            m_reportedAsleepIds.erase(netId);

            auto orderIt = std::find(
                m_ownershipOrder.begin(), m_ownershipOrder.end(), netId
            );
            if (orderIt != m_ownershipOrder.end()) {
                m_ownershipOrder.erase(orderIt);
            }
        } else if (body->isActive()) {
            m_reportedAsleepIds.erase(netId);
            outgoingStates.push_back(
                m_physics.captureNetworkState(netId, body)
            );
        } else if (m_reportedAsleepIds.insert(netId).second) {
            outgoingStates.push_back(
                m_physics.captureNetworkState(netId, body)
            );
        }
    }

    outgoingStates.push_back(
        m_physics.captureNetworkState(m_playerId, playerBody)
    );

    return outgoingStates;
}

} // namespace game::multiplayer