#pragma once

#include "../types/types.hpp"

#include "network/packets.hpp"

#include <btBulletDynamicsCommon.h>

#include <atomic>
#include <map>
#include <mutex>
#include <tuple>
#include <unordered_map>

namespace game::physics {

class Physics {
public:
    Physics(const std::shared_ptr<types::Workspace>& workspace);
    ~Physics();

    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    btDiscreteDynamicsWorld* getDynamicsWorld() const {
        return m_dynamicsWorld;
    }

    btRigidBody*
    createRigidBodyPart(types::Part* part, bool registerNetworked = true);

    btRigidBody* createRigidBodyRig(types::Rig* rig);

    void destroyRigidBody(types::Part* part);

    void rebindNetworkId(types::Part* part);

    std::recursive_mutex& getWorldMutex() {
        return m_worldMutex;
    }

    net::packets::PhysicalObjectState
    captureNetworkState(uint32_t networkId, btRigidBody* body) const;

    std::vector<uint32_t> getBodyCollisions(btRigidBody* body);

    void applyNetworkState(const net::packets::PhysicalObjectState& state);

    void setBodyNetworkMode(btRigidBody* body, bool isLocalOwner);

    void update(float deltaTime);

    void start(bool snapshot = false);
    void stop();
    bool isRunning() const {
        return m_running;
    }

private:
    struct BodyState {
        btTransform transform;
        btVector3 linearVelocity;
        btVector3 angularVelocity;
    };

    using ShapeKey = std::tuple<enums::PartShape, float, float, float>;

    static constexpr float kNetworkInterpolationSpeed = 15.0f;

    btDefaultCollisionConfiguration* m_collisionConfiguration;
    btCollisionDispatcher* m_dispatcher;
    btBroadphaseInterface* m_overlappingPairCache;
    btSequentialImpulseConstraintSolver* m_solver;

    btDiscreteDynamicsWorld* m_dynamicsWorld;

    std::map<ShapeKey, btCollisionShape*> m_shapeCache;

    std::recursive_mutex m_worldMutex;

    std::mutex m_networkTargetsMutex;
    std::unordered_map<uint32_t, net::packets::PhysicalObjectState>
        m_networkTargets;

    std::mutex m_bodiesMutex;
    std::unordered_map<uint32_t, btRigidBody*> m_bodies;

    std::unordered_map<btRigidBody*, float> m_bodyMasses;

    std::atomic<bool> m_running{false};
    std::unordered_map<btRigidBody*, BodyState> m_savedStates;

    btCylinderShape* createCylinderShape(const glm::vec3& size);
    btConvexHullShape* createWedgeShape(const glm::vec3& size);

    btCollisionShape*
    getOrCreateShape(enums::PartShape shape, const glm::vec3& size);

    void bindToWorkspace(const std::shared_ptr<types::Workspace>& workspace);

    void updatePartShape(types::Part* part, btRigidBody* body);

    void step(float dt);
    void stepNetworkInterpolation(float dt);
};

} // namespace game::physics