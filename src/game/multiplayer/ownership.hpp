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
    Ownership(physics::Physics& physics, net::Client& client);

    Ownership(const Ownership&) = delete;
    Ownership& operator=(const Ownership&) = delete;

    std::vector<net::packets::PhysicalObjectState> update(
        const std::shared_ptr<prefabs::Rig>& localRig,
        const std::unordered_map<uint32_t, std::shared_ptr<types::Part>>& networkParts);

private:
    struct SearchNode {
        uint32_t netId;
        int depth;
    };
    
    static constexpr size_t kMaxLocallyOwnedObjects = 32;
    static constexpr int kMaxCollisionDepth = 3;
    static constexpr float kReleaseDistanceSq = 15.0f * 15.0f;

    physics::Physics& m_physics;
    net::Client& m_client;

    std::deque<uint32_t> m_ownershipOrder;
    std::unordered_set<uint32_t> m_reportedAsleepIds;
};

} // namespace game::multiplayer