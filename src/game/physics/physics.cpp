#include "physics.hpp"

#include "model_motion_state.hpp"
#include "part_motion_state.hpp"

namespace game::physics {

Physics::Physics(const std::shared_ptr<types::Workspace>& workspace) {
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_overlappingPairCache = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);
    m_dynamicsWorld->setGravity(btVector3(0.0f, workspace->getGravity(), 0.0f));

    bindToWorkspace(workspace);
}

Physics::~Physics() {
    delete m_dynamicsWorld;

    delete m_solver;
    delete m_overlappingPairCache;
    delete m_dispatcher;
    delete m_collisionConfiguration;
}

void Physics::bindToWorkspace(const std::shared_ptr<types::Workspace>& workspace) {
    workspace->onPropertyChanged([this](std::shared_ptr<types::Workspace> changed) {
        m_dynamicsWorld->setGravity(btVector3(0.0f, changed->getGravity(), 0.0f));
    });

    workspace->onDescendantDestroy([this](std::shared_ptr<types::Instance> destroyed) {
        if (destroyed->getType() == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(destroyed);
            
            if (auto rigidBody = part->getRigidBody()) {
                m_dynamicsWorld->removeRigidBody(rigidBody);

                {
                    std::lock_guard<std::mutex> lock(m_bodiesMutex);
                    m_bodies.erase(part->getNetworkId());
                    m_bodyMasses.erase(rigidBody);
                }

                {
                    std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
                    m_networkTargets.erase(part->getNetworkId());
                }

                if (auto* ms = static_cast<PartMotionState*>(rigidBody->getMotionState())) {
                    ms->detachPart();
                }

                part->setRigidBody(nullptr);

                delete rigidBody->getMotionState();
                delete rigidBody;
            }
        }
    });
}

btCylinderShape* Physics::createCylinderShape(types::Part* part) {
    auto size = part->getSize();

    btScalar radius = size.x * 0.5f;
    btScalar halfHeight = size.y * 0.5f;

    return new btCylinderShape(btVector3(radius, halfHeight, radius));
}

btConvexHullShape* Physics::createWedgeShape(types::Part* part) {
    btConvexHullShape* wedgeShape = new btConvexHullShape();

    auto size = part->getSize();

    btScalar halfW = size.x * 0.5f;
    btScalar halfH = size.y * 0.5f;
    btScalar halfD = size.z * 0.5f;

    wedgeShape->addPoint(btVector3(-halfW, -halfH, -halfD));
    wedgeShape->addPoint(btVector3(halfW, -halfH, -halfD));
    wedgeShape->addPoint(btVector3(halfW, -halfH, halfD));
    wedgeShape->addPoint(btVector3(-halfW, -halfH, halfD));

    wedgeShape->addPoint(btVector3(-halfW, halfH, halfD));
    wedgeShape->addPoint(btVector3(halfW, halfH, halfD));

    wedgeShape->initializePolyhedralFeatures();

    return wedgeShape;
}

btRigidBody* Physics::createRigidBodyPart(types::Part* part) {
    btCollisionShape* shape = nullptr;

    glm::vec3 size = part->getSize();
    btVector3 halfExtents(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f);

    switch (part->getShape()) {
    case enums::PartType::Block:
        shape = new btBoxShape(halfExtents);
        break;
    case enums::PartType::Ball:
        shape = new btSphereShape(size.x / 2.0f);
        break;
    case enums::PartType::Cylinder:
        shape = createCylinderShape(part);
        break;
    case enums::PartType::Wedge:
        shape = createWedgeShape(part);
        break;
    case enums::PartType::Head:
        shape = createCylinderShape(part);
        break;
    default:
        shape = new btBoxShape(halfExtents);
        break;
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

    body->setSleepingThresholds(0.6f, 0.8f);
    body->setDeactivationTime(1.0f);

    m_dynamicsWorld->addRigidBody(body);

    part->setRigidBody(body);

    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        m_bodies[part->getNetworkId()] = body;
        m_bodyMasses[body] = mass;
    }

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

    body->setSleepingThresholds(0.6f, 0.8f);
    body->setDeactivationTime(1.0f);

    m_dynamicsWorld->addRigidBody(body);

    rootPart->setRigidBody(body);

    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        m_bodies[rootPart->getNetworkId()] = body;
        m_bodyMasses[body] = mass;
    }

    return body;
}

net::packets::PhysicalObjectState Physics::captureNetworkState(uint32_t networkId, btRigidBody* body) const {
    net::packets::PhysicalObjectState state{};
    state.networkId = networkId;

    const btTransform& transform = body->getWorldTransform();
    const btVector3& origin = transform.getOrigin();
    btQuaternion rotation = transform.getRotation();

    state.position = glm::vec3(origin.x(), origin.y(), origin.z());
    state.rotation = glm::quat(rotation.w(), rotation.x(), rotation.y(), rotation.z());

    const btVector3& linearVelocity = body->getLinearVelocity();
    const btVector3& angularVelocity = body->getAngularVelocity();

    state.linearVelocity = glm::vec3(linearVelocity.x(), linearVelocity.y(), linearVelocity.z());
    state.angularVelocity = glm::vec3(angularVelocity.x(), angularVelocity.y(), angularVelocity.z());

    return state;
}

std::vector<uint32_t> Physics::getBodyCollisions(btRigidBody* body) {
    std::vector<uint32_t> collidedNetworkIds;
    if (!body)
        return collidedNetworkIds;

    int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold
            = m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        if (contactManifold->getNumContacts() == 0)
            continue;

        const btCollisionObject* objA = contactManifold->getBody0();
        const btCollisionObject* objB = contactManifold->getBody1();

        if (objA == body || objB == body) {
            const btCollisionObject* collidedObj = (objA == body) ? objB : objA;
            const btRigidBody* collidedBody = btRigidBody::upcast(collidedObj);

            if (collidedBody) {
                for (const auto& [id, body] : m_bodies) {
                    if (body == collidedBody) {
                        collidedNetworkIds.push_back(id);
                        break;
                    }
                }
            }
        }
    }

    return collidedNetworkIds;
}

void Physics::applyNetworkState(const net::packets::PhysicalObjectState& state) {
    std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
    m_networkTargets[state.networkId] = state;
}

void Physics::stepNetworkInterpolation(float dt) {
    std::lock_guard<std::mutex> lock(m_networkTargetsMutex);

    if (m_networkTargets.empty())
        return;

    float alpha = 1.0f - glm::exp(-kNetworkInterpolationSpeed * dt);

    for (auto& [id, target] : m_networkTargets) {
        auto it = m_bodies.find(id);
        if (it == m_bodies.end() || !it->second)
            continue;

        btRigidBody* body = it->second;

        if (body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) {
            const btTransform& transform = body->getWorldTransform();
            const btVector3& currentOrigin = transform.getOrigin();
            btQuaternion currentRotation = transform.getRotation();

            glm::vec3 currentPosition(currentOrigin.x(), currentOrigin.y(), currentOrigin.z());
            glm::quat currentQuat(
                currentRotation.w(), currentRotation.x(), currentRotation.y(), currentRotation.z());

            glm::vec3 newPosition = glm::mix(currentPosition, target.position, alpha);
            glm::quat newRotation = glm::slerp(currentQuat, target.rotation, alpha);

            btTransform newTransform;
            newTransform.setOrigin(btVector3(newPosition.x, newPosition.y, newPosition.z));
            newTransform.setRotation(
                btQuaternion(newRotation.x, newRotation.y, newRotation.z, newRotation.w));

            body->setWorldTransform(newTransform);
            if (body->getMotionState()) {
                body->getMotionState()->setWorldTransform(newTransform);
            }
        }

        if (!(body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT)) {
            body->activate(true);
        }
    }
}

void Physics::setBodyNetworkMode(btRigidBody* body, bool isLocalOwner) {
    if (!body)
        return;

    int flags = body->getCollisionFlags();
    bool currentlyKinematic = (flags & btCollisionObject::CF_KINEMATIC_OBJECT) != 0;
    bool wantKinematic = !isLocalOwner;

    if (currentlyKinematic != wantKinematic) {
        if (isLocalOwner) {
            body->setCollisionFlags(flags & ~btCollisionObject::CF_KINEMATIC_OBJECT);

            {
                std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
                for (const auto& [id, b] : m_bodies) {
                    if (b == body) {
                        auto targetIt = m_networkTargets.find(id);
                        if (targetIt != m_networkTargets.end()) {
                            const auto& lastState = targetIt->second;
                            body->setLinearVelocity(btVector3(
                                lastState.linearVelocity.x, lastState.linearVelocity.y,
                                lastState.linearVelocity.z));
                            body->setAngularVelocity(btVector3(
                                lastState.angularVelocity.x, lastState.angularVelocity.y,
                                lastState.angularVelocity.z));
                        }

                        m_networkTargets.erase(id);
                        break;
                    }
                }
            }

            btScalar mass = 10.0f;
            {
                std::lock_guard<std::mutex> lock(m_bodiesMutex);
                auto it = m_bodyMasses.find(body);
                if (it != m_bodyMasses.end())
                    mass = it->second;
            }

            btVector3 localInertia(0, 0, 0);
            if (body->getCollisionShape())
                body->getCollisionShape()->calculateLocalInertia(mass, localInertia);
            body->setMassProps(mass, localInertia);
            body->updateInertiaTensor();

            body->forceActivationState(ACTIVE_TAG);
            body->activate(true);
        } else {
            net::packets::PhysicalObjectState localState{};
            bool found = false;
            {
                std::lock_guard<std::mutex> lock(m_bodiesMutex);
                for (const auto& [id, b] : m_bodies) {
                    if (b == body) {
                        localState = captureNetworkState(id, body);
                        found = true;
                        break;
                    }
                }
            }

            body->setCollisionFlags(flags | btCollisionObject::CF_KINEMATIC_OBJECT);
            body->setMassProps(0.0f, btVector3(0, 0, 0));

            body->forceActivationState(DISABLE_SIMULATION);

            if (found) {
                std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
                for (const auto& [id, b] : m_bodies) {
                    if (b == body) {
                        m_networkTargets[id] = localState;
                        break;
                    }
                }
            }
        }

        body->updateInertiaTensor();
        body->activate(true);
    }
}

void Physics::step(float deltaTime) {
    if (deltaTime > 0.1f)
        deltaTime = 0.1f;

    m_dynamicsWorld->stepSimulation(deltaTime, 5, 1.0f / 60.0f);

    std::vector<types::Part*> fallenParts;
    std::vector<types::Model*> fallenModels;
    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        for (const auto& [networkId, body] : m_bodies) {
            if (!body)
                continue;

            if (auto* motionState = static_cast<PartMotionState*>(body->getMotionState())) {
                if (motionState->isPendingDestroy()) {
                    if (types::Part* part = motionState->getPart()) {
                        fallenParts.push_back(part);
                        motionState->detachPart();
                    }
                }
            } else if (auto* motionState = static_cast<ModelMotionState*>(body->getMotionState())) {
                if (motionState->isPendingDestroy()) {
                    if (types::Model* model = motionState->getModel()) {
                        fallenModels.push_back(model);
                        motionState->detachModel();
                    }
                }
            }
        }
    }

    for (auto* part : fallenParts) {
        if (part) {
            part->destroy();
        }
    }

    for (auto* model : fallenModels) {
        if (model) {
            model->destroy();
        }
    }
}

} // namespace game::physics