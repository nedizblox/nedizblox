#pragma once

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
        std::shared_ptr<types::Workspace> workspace);
    ~Replicator();

    Replicator(const Replicator&) = delete;
    Replicator& operator=(const Replicator&) = delete;

    void start();
    void stop();

    void sendPhysicsState(const std::vector<net::packets::PhysicalObjectState>& states);

    net::Client& getClient() { return *m_client; }
    const std::unordered_map<uint32_t, std::shared_ptr<types::Instance>>& getNetworkInstances() const {
        return m_networkInstances;
    }
    const std::unordered_map<uint32_t, std::shared_ptr<prefabs::Rig>>& getNetworkRigs() const {
        return m_networkRigs;
    }

    void updateNetworkRigs(float dt);

private:
    struct PendingParenting {
        std::shared_ptr<types::Instance> instance;
        uint32_t parentId;
    };

    std::unique_ptr<net::Client> m_client;

    physics::Physics& m_physics;
    std::shared_ptr<types::Workspace> m_workspace;

    std::unordered_map<uint32_t, std::shared_ptr<types::Instance>> m_networkInstances;
    std::unordered_map<uint32_t, std::shared_ptr<prefabs::Rig>> m_networkRigs;

    std::shared_ptr<types::Instance> createInstanceByType(enums::InstanceType type);

    void setupCallbacks();
};

} // namespace game::multiplayer