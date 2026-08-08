#pragma once

#include "../managers/managers.hpp"
#include "../physics/physics.hpp"
#include "../prefabs/prefabs.hpp"
#include "../types/types.hpp"

#include "network/network.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace game::multiplayer {

class Replicator {
public:
    Replicator(
        const std::string& server, uint16_t port, const std::string& nickname, physics::Physics& physics,
        mngrs::InstanceManager& instanceManager, std::shared_ptr<types::Workspace> workspace);
    ~Replicator();

    Replicator(const Replicator&) = delete;
    Replicator& operator=(const Replicator&) = delete;

    void start();
    void stop();

    void sendPhysicsState(const std::vector<net::packets::PhysicalObjectState>& states);

    net::Client& getClient() { return *m_client; }
    const std::unordered_map<uint32_t, std::shared_ptr<types::Part>>& getNetworkParts() const {
        return m_networkParts;
    }
    const std::unordered_map<uint32_t, std::shared_ptr<prefabs::Rig>>& getNetworkRigs() const {
        return m_networkRigs;
    }

    void updateNetworkRigs(float dt);

private:
    std::unique_ptr<net::Client> m_client;

    physics::Physics& m_physics;
    mngrs::InstanceManager& m_instanceManager;
    std::shared_ptr<types::Workspace> m_workspace;

    std::unordered_map<uint32_t, std::shared_ptr<types::Part>> m_networkParts;
    std::unordered_map<uint32_t, std::shared_ptr<prefabs::Rig>> m_networkRigs;

    void setupCallbacks();
};

} // namespace game::multiplayer