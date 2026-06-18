#include "physics.hpp"

#include "model_motion_state.hpp"
#include "part_motion_state.hpp"

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
    info.m_splitImpulse = 1;
    info.m_splitImpulsePenetrationThreshold = -0.02f;
    info.m_solverMode &= ~SOLVER_USE_2_FRICTION_DIRECTIONS;
    info.m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
    info.m_linearSlop = 0.05f;
    info.m_warmstartingFactor = 0.85f;
}

Physics::~Physics() {
    delete m_dynamicsWorld;

    delete m_solver;
    delete m_overlappingPairCache;
    delete m_dispatcher;
    delete m_collisionConfiguration;
}

btRigidBody* Physics::createRigidBodyPart(types::Part* part) {
    btCollisionShape* shape = nullptr;

    glm::vec3 size = part->getSize();
    btVector3 halfExtents(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f);

    auto type = part->getShape();
    if (type == enums::PartType::Block) {
        shape = new btBoxShape(halfExtents);
    } else if (type == enums::PartType::Ball) {
        shape = new btSphereShape(size.x / 2.0f);
    } else if (type == enums::PartType::Capsule) {
        float radius = size.x / 2.0f;
        float cylinderHeight = size.y - (2.0f * radius);

        if (cylinderHeight < 0.0f) {
            cylinderHeight = 0.0f;
        }

        shape = new btCapsuleShape(radius, cylinderHeight);
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

    body->setSleepingThresholds(0.8f, 1.0f);
    body->setDeactivationTime(0.5f);

    m_dynamicsWorld->addRigidBody(body);

    return body;
}

btRigidBody* Physics::createRigidBodyModel(types::Model* model, types::Part* rootPart) {
    btCollisionShape* shape = nullptr;

    glm::vec3 size = rootPart->getSize();
    btVector3 halfExtents(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f);

    shape = new btBoxShape(halfExtents);
    shape->setLocalScaling(btVector3(1.0f, 1.0f, 1.0f));

    float mass = 10.0f;
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    shape->calculateLocalInertia(mass, inertia);

    btTransform startTrans;
    startTrans.setIdentity();

    glm::mat4 pivot = model->getPivot();
    glm::vec3 p = glm::vec3(pivot[3]);
    startTrans.setOrigin(btVector3(p.x, p.y, p.z));

    glm::quat o = glm::toQuat(pivot);
    btQuaternion quat(o.x, o.y, o.z, o.w);
    startTrans.setRotation(quat);

    auto* motionState = new ModelMotionState(startTrans, model);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, motionState, shape, inertia);
    btRigidBody* body = new btRigidBody(ci);

    body->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
    body->setActivationState(DISABLE_DEACTIVATION);

    m_dynamicsWorld->addRigidBody(body);

    return body;
}

void Physics::step(float deltaTime) {
    float dt = glm::min(deltaTime, 0.1f);

    m_dynamicsWorld->stepSimulation(dt, 1, 1.0f / 60.0f);
}

} // namespace game::physics