#pragma once

#include "../managers/humanoid_manager.hpp"
#include "../physics/physics.hpp"
#include "../types/types.hpp"

#include "network/network.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace game::multiplayer {

class ClientReplicator {
public:
    ClientReplicator(
        const std::string& server, uint16_t port, const std::string& nickname,
        physics::Physics& physics, mngrs::HumanoidManager& humanoids,
        std::shared_ptr<types::Workspace> workspace
    );
    ~ClientReplicator();

    ClientReplicator(const ClientReplicator&) = delete;
    ClientReplicator& operator=(const ClientReplicator&) = delete;

    void start();
    void stop();

    uint32_t getPlayerId() const {
        if (!m_offlineMode)
            return m_client->getPlayerId();
        else
            return 0;
    }

    bool isOfflineMode() const {
        return m_offlineMode;
    }

    void sendPhysicsState(
        const std::vector<net::packets::PhysicalObjectState>& states
    );

    net::Client* getClient() {
        return m_client.get();
    }
    const std::unordered_map<uint32_t, std::shared_ptr<types::Instance>>&
    getNetworkInstances() const {
        return m_networkInstances;
    }
    const std::unordered_map<uint32_t, std::shared_ptr<types::Rig>>&
    getNetworkRigs() const {
        return m_networkRigs;
    }

private:
    struct PendingParenting {
        std::shared_ptr<types::Instance> instance;
        uint32_t parentId;
    };

    std::unique_ptr<net::Client> m_client;

    bool m_offlineMode = false;

    physics::Physics& m_physics;
    mngrs::HumanoidManager& m_humanoids;
    std::shared_ptr<types::Workspace> m_workspace;

    std::unordered_map<uint32_t, std::shared_ptr<types::Instance>>
        m_networkInstances;
    std::unordered_map<uint32_t, std::shared_ptr<types::Rig>> m_networkRigs;

    void setupCallbacks();
    void addRemotePlayer(uint32_t playerId, const std::string& nickname);
};

} // namespace game::multiplayer