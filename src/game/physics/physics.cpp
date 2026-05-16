#include "physics.hpp"

#include "part_motion_state.hpp"

#include <glm/glm.hpp>

namespace game::physics {

Physics::Physics(float gravity) {
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_overlappingPairCache = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

    m_dynamicsWorld->setGravity(btVector3(0.0f, gravity, 0.0f));

    auto& info = m_dynamicsWorld->getSolverInfo();
    info.m_numIterations = 4;
}

Physics::~Physics() {
    delete m_dynamicsWorld;

    delete m_solver;
    delete m_overlappingPairCache;
    delete m_dispatcher;
    delete m_collisionConfiguration;
}

btRigidBody* Physics::createRigidBody(types::Part* part) {
    static btCollisionShape* shape = nullptr;

    glm::vec3 size = part->getSize();
    btVector3 halfExtents(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f);

    auto type = part->getShape();
    if (type == enums::PartType::Block) {
        shape = new btBoxShape(halfExtents);
    } else if (type == enums::PartType::Ball) {
        shape = new btSphereShape(size.x / 2.0f);
    } else {
        shape = new btBoxShape(halfExtents);
    }

    shape->setLocalScaling(btVector3(1.0f, 1.0f, 1.0f));

    float mass = part->getAnchored() ? 0.0f : (size.x * size.y * size.z);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (mass > 0.0f)
        shape->calculateLocalInertia(mass, inertia);

    btTransform startTrans;
    startTrans.setIdentity();

    glm::vec3 p = part->getPosition();
    startTrans.setOrigin(btVector3(p.x, p.y, p.z));

    glm::quat o = part->getOrientation();
    btQuaternion quat(o.x, o.y, o.z, o.w);
    startTrans.setRotation(quat);

    auto* motionState = new PartMotionState(startTrans, part);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, motionState, shape, inertia);
    btRigidBody* body = new btRigidBody(ci);

    m_dynamicsWorld->addRigidBody(body);

    return body;
}

void Physics::step(float deltaTime) { m_dynamicsWorld->stepSimulation(deltaTime, 1, 1.0f / 60.0f); }

} // namespace game::physics