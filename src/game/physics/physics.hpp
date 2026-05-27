#pragma once

#include "../types/types.hpp"

#include <btBulletDynamicsCommon.h>

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

    void step(float deltaTime);

private:
    btDefaultCollisionConfiguration* m_collisionConfiguration;
    btCollisionDispatcher* m_dispatcher;
    btBroadphaseInterface* m_overlappingPairCache;
    btSequentialImpulseConstraintSolver* m_solver;

    btDiscreteDynamicsWorld* m_dynamicsWorld;
};

} // namespace game::physics