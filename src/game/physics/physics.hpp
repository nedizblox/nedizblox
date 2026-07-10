#pragma once

#include "../types/types.hpp"
#include "network/packets.hpp"

#include <btBulletDynamicsCommon.h>

#include <mutex>
#include <unordered_map>

namespace game::physics {

class Physics {
public:
    Physics(float gravity);
    ~Physics();

    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    btDiscreteDynamicsWorld* getDynamicsWorld() const { return m_dynamicsWorld; }

    btRigidBody* createRigidBodyPart(types::Part* part);
    btRigidBody* createRigidBodyModel(types::Model* model, types::Part* rootPart);

    net::packets::PhysicalObjectState captureNetworkState(uint32_t networkId, btRigidBody* body) const;

    std::vector<uint32_t> getBodyCollisions(btRigidBody* body);

    void applyNetworkState(const net::packets::PhysicalObjectState& state);

    void stepNetworkInterpolation(float dt);

    void setBodyNetworkMode(btRigidBody* body, bool isLocalOwner);

    void step(float deltaTime);

private:
    static constexpr float kNetworkInterpolationSpeed = 15.0f;

    btDefaultCollisionConfiguration* m_collisionConfiguration;
    btCollisionDispatcher* m_dispatcher;
    btBroadphaseInterface* m_overlappingPairCache;
    btSequentialImpulseConstraintSolver* m_solver;

    btDiscreteDynamicsWorld* m_dynamicsWorld;

    std::mutex m_networkTargetsMutex;
    std::unordered_map<uint32_t, net::packets::PhysicalObjectState> m_networkTargets;

    std::mutex m_bodiesMutex;
    std::unordered_map<uint32_t, btRigidBody*> m_bodies;
};

} // namespace game::physics