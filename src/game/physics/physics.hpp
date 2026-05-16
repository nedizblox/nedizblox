#pragma one

#include "../types/types.hpp"

#include <btBulletDynamicsCommon.h>

namespace game::physics {

class Physics {
public:
    Physics(float gravity);
    ~Physics();

    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    btRigidBody* createRigidBody(types::Part* part);

    void step(float deltaTime);

private:
    btDefaultCollisionConfiguration* m_collisionConfiguration;
    btCollisionDispatcher* m_dispatcher;
    btBroadphaseInterface* m_overlappingPairCache;
    btSequentialImpulseConstraintSolver* m_solver;

    btDiscreteDynamicsWorld* m_dynamicsWorld;
};

} // namespace game::physics