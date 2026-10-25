#pragma once

#include "../physics/physics.hpp"
#include "../types/types.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace game::mngrs {

class HumanoidManager {
public:
    static constexpr float kRespawnHeight = -500.0f;
    static constexpr float kRespawnDelay = 3.0f;
    static constexpr float kInputDeadzone = 0.05f;
    static constexpr float kTurnSpeed = 10.0f;

    static constexpr float kGroundProbeRadius = 0.8f;
    static constexpr float kGroundProbeTolerance = 0.2f;

    static constexpr float kSpawnRetryInterval = 0.5f;
    static constexpr float kSpawnAreaFraction = 0.8f;
    static constexpr float kSpawnClearance = 0.5f;

    static constexpr uint32_t kAdoptedIdBase = 0x80000000u;

    static constexpr float kRagdollScatterSpeed = 4.0f;
    static constexpr float kRagdollSpinSpeed = 5.0f;

    HumanoidManager(
        physics::Physics& physics, std::shared_ptr<types::Workspace> workspace
    );
    ~HumanoidManager();

    HumanoidManager(const HumanoidManager&) = delete;
    HumanoidManager& operator=(const HumanoidManager&) = delete;

    std::shared_ptr<types::Rig> createHumanoid(
        const std::string& name, uint32_t networkId, bool isLocal,
        const glm::vec3& spawnPosition = glm::vec3(0.0f, 10.0f, 0.0f)
    );

    void removeHumanoid(uint32_t networkId);

    bool hasHumanoid(uint32_t networkId) const;
    std::shared_ptr<types::Rig> getRig(uint32_t networkId) const;
    btRigidBody* getRigidBody(uint32_t networkId) const;
    bool isDead(uint32_t networkId) const;

    void move(uint32_t networkId, const glm::vec2& axis, float phi);

    bool jump(uint32_t networkId);

    void damage(uint32_t networkId, uint32_t amount);
    void kill(uint32_t networkId);

    void update(float dt);

private:
    struct Limb {
        std::shared_ptr<types::Part> part;
        glm::vec3 offset;
    };

    struct State {
        std::shared_ptr<types::Rig> rig;
        std::shared_ptr<types::Part> rootPart;
        std::shared_ptr<types::Humanoid> humanoid;
        std::vector<Limb> limbs;

        glm::vec3 spawnPosition{0.0f};
        glm::vec3 moveDir{0.0f};

        bool isLocal = false;
        bool dead = false;
        float respawnTimer = 0.0f;

        bool awaitingSpawn = false;
        float spawnRetryTimer = 0.0f;
    };

    physics::Physics& m_physics;
    std::shared_ptr<types::Workspace> m_workspace;

    mutable std::recursive_mutex m_mutex;
    std::unordered_map<uint32_t, State> m_states;

    std::mt19937 m_rng;

    std::shared_ptr<bool> m_aliveToken = std::make_shared<bool>(true);

    std::vector<std::weak_ptr<types::Rig>> m_pendingRigs;
    uint32_t m_nextAdoptedId = kAdoptedIdBase;

    void watchWorkspace();
    void onInstanceAdded(const std::shared_ptr<types::Instance>& instance);
    void onInstanceDestroyed(const std::shared_ptr<types::Instance>& instance);

    void queueRig(const std::shared_ptr<types::Rig>& rig);
    bool tryAdoptRig(const std::shared_ptr<types::Rig>& rig);
    void retryPendingRigs();
    void forgetRig(const types::Rig* rig);

    bool isTracked(const types::Rig* rig) const;
    static bool isRigComplete(types::Rig& rig);
    uint32_t allocateAdoptedId();

    void createBody(State& state);
    void syncParts(State& state);

    void updateLocalMovement(State& state, btRigidBody* body, float dt);
    bool isGrounded(const State& state, btRigidBody* body);
    void teleportToSpawn(State& state, btRigidBody* body);

    bool findSpawnPosition(const State& state, glm::vec3& outPosition);

    void killState(State& state);
    void respawnState(State& state);

    float randomRange(float minValue, float maxValue);
};

} // namespace game::mngrs