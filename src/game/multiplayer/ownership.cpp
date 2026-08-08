#include "ownership.hpp"

#include <algorithm>

namespace game::multiplayer {

Ownership::Ownership(physics::Physics& physics, net::Client& client) :
    m_physics(physics), m_client(client) {}

std::vector<net::packets::PhysicalObjectState> Ownership::update(
    const std::shared_ptr<prefabs::Rig>& localRig,
    const std::unordered_map<uint32_t, std::shared_ptr<types::Part>>& networkParts) {

    std::vector<net::packets::PhysicalObjectState> outgoingStates;

    if (!localRig)
        return outgoingStates;

    btRigidBody* playerBody = localRig->getRigidBody();
    if (!playerBody)
        return outgoingStates;

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

        auto it = networkParts.find(current.netId);
        if (it == networkParts.end() || !it->second || it->second->getAnchored())
            continue;

        candidates.insert(current.netId);

        if (current.depth < kMaxCollisionDepth) {
            btRigidBody* b = it->second->getRigidBody();
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
        auto it = networkParts.find(netId);
        if (it == networkParts.end() || !it->second)
            continue;

        btRigidBody* b = it->second->getRigidBody();
        if (!b)
            continue;

        auto orderIt = std::find(m_ownershipOrder.begin(), m_ownershipOrder.end(), netId);

        if (orderIt == m_ownershipOrder.end()) {
            if (m_ownershipOrder.size() >= kMaxLocallyOwnedObjects) {
                uint32_t oldestId = m_ownershipOrder.front();
                m_ownershipOrder.pop_front();
                m_reportedAsleepIds.erase(oldestId);

                auto oldIt = networkParts.find(oldestId);
                if (oldIt != networkParts.end() && oldIt->second) {
                    btRigidBody* oldBody = oldIt->second->getRigidBody();
                    if (oldBody) {
                        m_physics.setBodyNetworkMode(oldBody, false);
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

    std::vector<uint32_t> activeOwnerships(m_ownershipOrder.begin(), m_ownershipOrder.end());

    for (uint32_t netId : activeOwnerships) {
        auto it = networkParts.find(netId);
        if (it == networkParts.end() || !it->second)
            continue;

        btRigidBody* body = it->second->getRigidBody();
        if (!body)
            continue;

        bool isFarAway = false;
        btVector3 btPos = body->getWorldTransform().getOrigin();
        float distSq
            = glm::distance2(localRig->getPivotPosition(), glm::vec3(btPos.x(), btPos.y(), btPos.z()));

        if (distSq > kReleaseDistanceSq) {
            isFarAway = true;
        }

        bool isContactLost = (candidates.find(netId) == candidates.end());
        bool shouldRelease = isFarAway || (!body->isActive() && isContactLost);

        if (shouldRelease) {
            outgoingStates.push_back(m_physics.captureNetworkState(netId, body));
            m_physics.setBodyNetworkMode(body, false);
            m_reportedAsleepIds.erase(netId);

            auto orderIt = std::find(m_ownershipOrder.begin(), m_ownershipOrder.end(), netId);
            if (orderIt != m_ownershipOrder.end()) {
                m_ownershipOrder.erase(orderIt);
            }
        } else if (body->isActive()) {
            m_reportedAsleepIds.erase(netId);
            outgoingStates.push_back(m_physics.captureNetworkState(netId, body));
        } else if (m_reportedAsleepIds.insert(netId).second) {
            outgoingStates.push_back(m_physics.captureNetworkState(netId, body));
        }
    }

    outgoingStates.push_back(m_physics.captureNetworkState(m_client.getPlayerId(), playerBody));

    return outgoingStates;
}

} // namespace game::multiplayer