#pragma once

#include "network/network.hpp"

#include "../physics/physics.hpp"
#include "../prefabs/prefabs.hpp"
#include "../types/types.hpp"

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace game::multiplayer {

class Ownership {
public:
    static constexpr size_t kMaxLocallyOwnedObjects = 32;
    static constexpr int kMaxCollisionDepth = 3;
    static constexpr float kReleaseDistanceSq = 15.0f * 15.0f;

    static constexpr float kNetSendInterval = 1.0f / 20.0f;
    
    Ownership(physics::Physics& physics, net::Client& client);

    Ownership(const Ownership&) = delete;
    Ownership& operator=(const Ownership&) = delete;

    const std::deque<uint32_t>& getOwnedObjectIds() const { return m_ownershipOrder; }

    std::vector<net::packets::PhysicalObjectState> update(
        float dt, const std::shared_ptr<prefabs::Rig>& localRig,
        const std::unordered_map<uint32_t, std::shared_ptr<types::Instance>>& networkInstances);

private:
    struct SearchNode {
        uint32_t netId;
        int depth;
    };
    
    float m_netSendAccumulator = 0.0f;

    physics::Physics& m_physics;
    net::Client& m_client;

    std::deque<uint32_t> m_ownershipOrder;
    std::unordered_set<uint32_t> m_reportedAsleepIds;
};

} // namespace game::multiplayer