#include "physics.hpp"

#include "part_motion_state.hpp"
#include "rig_motion_state.hpp"

namespace game::physics {

namespace {

btQuaternion toBtQuat(const glm::quat& q) {
    const float len2 = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (!(len2 > 1e-8f))
        return btQuaternion::getIdentity();

    const float inv = 1.0f / glm::sqrt(len2);
    return btQuaternion(q.x * inv, q.y * inv, q.z * inv, q.w * inv);
}

} // namespace

Physics::Physics(const std::shared_ptr<types::Workspace>& workspace) {
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_overlappingPairCache = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(
        m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration
    );
    m_dynamicsWorld->setGravity(btVector3(0.0f, workspace->getGravity(), 0.0f));

    bindToWorkspace(workspace);
}

Physics::~Physics() {
    if (m_dynamicsWorld) {
        for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0;
             i--) {
            btCollisionObject* obj =
                m_dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);

            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }

            m_dynamicsWorld->removeCollisionObject(obj);
            delete obj;
        }
    }

    for (auto& [key, shape] : m_shapeCache) {
        delete shape;
    }

    delete m_dynamicsWorld;

    delete m_solver;
    delete m_overlappingPairCache;
    delete m_dispatcher;
    delete m_collisionConfiguration;
}

void Physics::bindToWorkspace(
    const std::shared_ptr<types::Workspace>& workspace
) {
    workspace->onPropertyChanged(
        [this](std::shared_ptr<types::Workspace> changed)
        {
            m_dynamicsWorld->setGravity(
                btVector3(0.0f, changed->getGravity(), 0.0f)
            );
        }
    );

    auto ensureBody = [this](const std::shared_ptr<types::Instance>& instance)
    {
        if (!instance || instance->getType() != enums::InstanceType::Part)
            return;

        auto part = std::static_pointer_cast<types::Part>(instance);
        if (part->getRigidBody())
            return;

        if (dynamic_cast<types::Rig*>(part->getParent().get()))
            return;

        createRigidBodyPart(part.get());
    };

    workspace->onDescendantAdded(
        [ensureBody](std::shared_ptr<types::Instance> added)
        {
            if (!added)
                return;

            ensureBody(added);

            for (const auto& descendant : added->getDescendants()) {
                ensureBody(descendant);
            }
        }
    );

    for (const auto& descendant : workspace->getDescendants()) {
        ensureBody(descendant);
    }

    workspace->onDescendantPropertyChanged(
        [this](std::shared_ptr<types::Instance> changed)
        {
            if (!changed || changed->getType() != enums::InstanceType::Part)
                return;

            auto part = std::static_pointer_cast<types::Part>(changed);
            btRigidBody* body = part->getRigidBody();
            if (!body)
                return;

            if (!dynamic_cast<types::Rig*>(part->getParent().get()))
                updatePartShape(part.get(), body);

            glm::vec3 p = part->getPosition();
            glm::quat o = part->getOrientation();
            const btQuaternion targetRotation = toBtQuat(o);

            const btTransform& current = body->getWorldTransform();
            const btVector3& currentOrigin = current.getOrigin();
            btQuaternion currentRotation = current.getRotation();

            glm::vec3 positionDelta(
                p.x - currentOrigin.x(), p.y - currentOrigin.y(),
                p.z - currentOrigin.z()
            );
            float rotationDot = glm::abs(
                targetRotation.x() * currentRotation.x() +
                targetRotation.y() * currentRotation.y() +
                targetRotation.z() * currentRotation.z() +
                targetRotation.w() * currentRotation.w()
            );

            bool positionChanged =
                glm::dot(positionDelta, positionDelta) > 1e-6f;
            bool rotationChanged = rotationDot < 0.999999f;

            if (!positionChanged && !rotationChanged)
                return;

            btTransform transform;
            transform.setOrigin(btVector3(p.x, p.y, p.z));
            transform.setRotation(targetRotation);

            body->setWorldTransform(transform);
            if (body->getMotionState()) {
                body->getMotionState()->setWorldTransform(transform);
            }

            body->clearForces();
            body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
            body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
            body->activate(true);

            if (body->getCollisionFlags() &
                btCollisionObject::CF_KINEMATIC_OBJECT) {
                std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
                auto& target = m_networkTargets[part->getNetworkId()];
                target.networkId = part->getNetworkId();
                target.position = p;
                target.rotation = o;
                target.linearVelocity = glm::vec3(0.0f);
                target.angularVelocity = glm::vec3(0.0f);
            }
        }
    );

    workspace->onDescendantDestroy(
        [this](std::shared_ptr<types::Instance> destroyed)
        {
            if (destroyed->getType() == enums::InstanceType::Part) {
                destroyRigidBody(static_cast<types::Part*>(destroyed.get()));
            }
        }
    );
}

btCylinderShape* Physics::createCylinderShape(const glm::vec3& size) {
    btScalar radius = size.x * 0.5f;
    btScalar halfHeight = size.y * 0.5f;

    return new btCylinderShape(btVector3(radius, halfHeight, radius));
}

btConvexHullShape* Physics::createWedgeShape(const glm::vec3& size) {
    btConvexHullShape* wedgeShape = new btConvexHullShape();

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

btCollisionShape*
Physics::getOrCreateShape(enums::PartShape shape, const glm::vec3& size) {
    auto key = std::make_tuple(shape, size.x, size.y, size.z);

    auto it = m_shapeCache.find(key);
    if (it != m_shapeCache.end()) {
        return it->second;
    }

    btCollisionShape* newShape = nullptr;
    btVector3 halfExtents(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);

    switch (shape) {
        case enums::PartShape::Block:
            newShape = new btBoxShape(halfExtents);
            break;
        case enums::PartShape::Ball:
            newShape = new btSphereShape(size.x / 2.0f);
            break;
        case enums::PartShape::Cylinder:
            newShape = createCylinderShape(size);
            break;
        case enums::PartShape::Wedge:
            newShape = createWedgeShape(size);
            break;
        case enums::PartShape::Head:
            newShape = createCylinderShape(size);
            break;
        default:
            newShape = new btBoxShape(halfExtents);
            break;
    }

    m_shapeCache[key] = newShape;
    return newShape;
}

btRigidBody*
Physics::createRigidBodyPart(types::Part* part, bool registerNetworked) {
    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    if (btRigidBody* existing = part->getRigidBody())
        return existing;

    glm::vec3 size = part->getSize();

    btCollisionShape* shape = getOrCreateShape(part->getShape(), size);
    shape->setLocalScaling(btVector3(1.0f, 1.0f, 1.0f));

    float mass = part->getAnchored() ? 0.0f : (size.x * size.y * size.z);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (mass > 0.0f)
        shape->calculateLocalInertia(mass, inertia);

    btTransform startTrans;
    startTrans.setIdentity();

    glm::vec3 p = part->getPosition();
    startTrans.setOrigin(btVector3(p.x, p.y, p.z));

    startTrans.setRotation(toBtQuat(part->getOrientation()));

    auto* motionState = new PartMotionState(startTrans, part);

    btRigidBody::btRigidBodyConstructionInfo ci(
        mass, motionState, shape, inertia
    );
    btRigidBody* body = new btRigidBody(ci);

    body->setSleepingThresholds(0.6f, 0.8f);
    body->setDeactivationTime(0.1f);

    m_dynamicsWorld->addRigidBody(body);

    part->setRigidBody(body);

    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        if (registerNetworked)
            m_bodies[part->getNetworkId()] = body;
        m_bodyMasses[body] = mass;
    }

    return body;
}

void Physics::updatePartShape(types::Part* part, btRigidBody* body) {
    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    const glm::vec3 size = part->getSize();
    btCollisionShape* desiredShape = getOrCreateShape(part->getShape(), size);
    const float desiredMass =
        part->getAnchored() ? 0.0f : (size.x * size.y * size.z);

    float currentMass = -1.0f;
    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        auto it = m_bodyMasses.find(body);
        if (it != m_bodyMasses.end())
            currentMass = it->second;
    }

    const bool shapeChanged = body->getCollisionShape() != desiredShape;
    const bool massChanged = std::abs(currentMass - desiredMass) > 1e-6f;
    if (!shapeChanged && !massChanged)
        return;

    m_dynamicsWorld->removeRigidBody(body);

    if (shapeChanged) {
        desiredShape->setLocalScaling(btVector3(1.0f, 1.0f, 1.0f));
        body->setCollisionShape(desiredShape);
    }

    const bool kinematic = (body->getCollisionFlags() &
                            btCollisionObject::CF_KINEMATIC_OBJECT) != 0;
    if (!kinematic) {
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        if (desiredMass > 0.0f)
            desiredShape->calculateLocalInertia(desiredMass, inertia);

        body->setMassProps(desiredMass, inertia);
        body->updateInertiaTensor();
    }

    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        m_bodyMasses[body] = desiredMass;
    }

    m_dynamicsWorld->addRigidBody(body);
    body->activate(true);
}

void Physics::rebindNetworkId(types::Part* part) {
    btRigidBody* body = part ? part->getRigidBody() : nullptr;
    if (!body)
        return;

    std::lock_guard<std::mutex> lock(m_bodiesMutex);

    for (auto it = m_bodies.begin(); it != m_bodies.end();) {
        if (it->second == body)
            it = m_bodies.erase(it);
        else
            ++it;
    }

    m_bodies[part->getNetworkId()] = body;
}

void Physics::destroyRigidBody(types::Part* part) {
    if (!part)
        return;

    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    btRigidBody* rigidBody = part->getRigidBody();
    if (!rigidBody)
        return;

    m_dynamicsWorld->removeRigidBody(rigidBody);

    std::vector<uint32_t> removedIds;

    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);

        for (auto it = m_bodies.begin(); it != m_bodies.end();) {
            if (it->second == rigidBody) {
                removedIds.push_back(it->first);
                it = m_bodies.erase(it);
            } else {
                ++it;
            }
        }
        m_bodyMasses.erase(rigidBody);
    }

    if (!removedIds.empty()) {
        std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
        for (uint32_t id : removedIds)
            m_networkTargets.erase(id);
    }

    if (auto* ms = static_cast<BodyMotionState*>(rigidBody->getMotionState())) {
        ms->detach();
    }

    part->setRigidBody(nullptr);

    delete rigidBody->getMotionState();
    delete rigidBody;
}

btRigidBody* Physics::createRigidBodyRig(types::Rig* rig) {
    if (!rig)
        return nullptr;

    std::shared_ptr<types::Part> rootPart = rig->getRootPart();
    if (!rootPart)
        return nullptr;

    if (btRigidBody* existing = rootPart->getRigidBody())
        return existing;

    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    glm::vec3 size = rootPart->getSize();

    btCollisionShape* shape = getOrCreateShape(rootPart->getShape(), size);
    shape->setLocalScaling(btVector3(1.0f, 1.0f, 1.0f));

    float mass = 10.0f;
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    shape->calculateLocalInertia(mass, inertia);

    btTransform startTrans;
    startTrans.setIdentity();

    glm::mat4 pivot = rig->getPivot();
    glm::vec3 p = glm::vec3(pivot[3]);
    startTrans.setOrigin(btVector3(p.x, p.y, p.z));

    glm::quat o = glm::toQuat(pivot);
    btQuaternion quat(o.x, o.y, o.z, o.w);
    startTrans.setRotation(quat);

    auto* motionState = new RigMotionState(startTrans, rig);

    btRigidBody::btRigidBodyConstructionInfo ci(
        mass, motionState, shape, inertia
    );
    btRigidBody* body = new btRigidBody(ci);

    body->setSleepingThresholds(0.6f, 0.8f);
    body->setDeactivationTime(0.1f);

    body->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));
    body->setActivationState(DISABLE_DEACTIVATION);

    m_dynamicsWorld->addRigidBody(body);

    rootPart->setRigidBody(body);

    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        m_bodies[rootPart->getNetworkId()] = body;
        m_bodyMasses[body] = mass;
    }

    return body;
}

net::packets::PhysicalObjectState
Physics::captureNetworkState(uint32_t networkId, btRigidBody* body) const {
    net::packets::PhysicalObjectState state{};
    state.networkId = networkId;

    const btTransform& transform = body->getWorldTransform();
    const btVector3& origin = transform.getOrigin();
    btQuaternion rotation = transform.getRotation();

    state.position = glm::vec3(origin.x(), origin.y(), origin.z());
    state.rotation =
        glm::quat(rotation.w(), rotation.x(), rotation.y(), rotation.z());

    const btVector3& linearVelocity = body->getLinearVelocity();
    const btVector3& angularVelocity = body->getAngularVelocity();

    state.linearVelocity =
        glm::vec3(linearVelocity.x(), linearVelocity.y(), linearVelocity.z());
    state.angularVelocity = glm::vec3(
        angularVelocity.x(), angularVelocity.y(), angularVelocity.z()
    );

    return state;
}

std::vector<uint32_t> Physics::getBodyCollisions(btRigidBody* body) {
    std::vector<uint32_t> collidedNetworkIds;
    if (!body)
        return collidedNetworkIds;

    std::lock_guard<std::mutex> lock(m_bodiesMutex);

    int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold =
            m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        if (contactManifold->getNumContacts() == 0)
            continue;

        const btCollisionObject* objA = contactManifold->getBody0();
        const btCollisionObject* objB = contactManifold->getBody1();

        if (objA == body || objB == body) {
            const btCollisionObject* collidedObj = (objA == body) ? objB : objA;
            const btRigidBody* collidedBody = btRigidBody::upcast(collidedObj);

            if (collidedBody) {
                for (const auto& [id, other] : m_bodies) {
                    if (other == collidedBody) {
                        collidedNetworkIds.push_back(id);
                        break;
                    }
                }
            }
        }
    }

    return collidedNetworkIds;
}

void Physics::applyNetworkState(
    const net::packets::PhysicalObjectState& state
) {
    std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
    m_networkTargets[state.networkId] = state;
}

void Physics::setBodyNetworkMode(btRigidBody* body, bool isLocalOwner) {
    if (!body)
        return;

    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    int flags = body->getCollisionFlags();
    bool currentlyKinematic =
        (flags & btCollisionObject::CF_KINEMATIC_OBJECT) != 0;
    bool wantKinematic = !isLocalOwner;

    if (currentlyKinematic != wantKinematic) {
        if (isLocalOwner) {
            body->setCollisionFlags(
                flags & ~btCollisionObject::CF_KINEMATIC_OBJECT
            );

            {
                std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
                for (const auto& [id, b] : m_bodies) {
                    if (b == body) {
                        auto targetIt = m_networkTargets.find(id);
                        if (targetIt != m_networkTargets.end()) {
                            const auto& lastState = targetIt->second;
                            body->setLinearVelocity(btVector3(
                                lastState.linearVelocity.x,
                                lastState.linearVelocity.y,
                                lastState.linearVelocity.z
                            ));
                            body->setAngularVelocity(btVector3(
                                lastState.angularVelocity.x,
                                lastState.angularVelocity.y,
                                lastState.angularVelocity.z
                            ));
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
                body->getCollisionShape()->calculateLocalInertia(
                    mass, localInertia
                );
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

            body->setCollisionFlags(
                flags | btCollisionObject::CF_KINEMATIC_OBJECT
            );
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

void Physics::update(float deltaTime) {
    if (m_running) {
        step(deltaTime);
        stepNetworkInterpolation(deltaTime);
    }
}

void Physics::start(bool snapshot) {
    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    if (m_running)
        return;

    if (snapshot) {
        m_savedStates.clear();

        const btCollisionObjectArray& objects =
            m_dynamicsWorld->getCollisionObjectArray();
        for (int i = 0; i < objects.size(); i++) {
            btRigidBody* body = btRigidBody::upcast(objects[i]);
            if (!body)
                continue;

            m_savedStates[body] = BodyState{
                body->getWorldTransform(), body->getLinearVelocity(),
                body->getAngularVelocity()
            };
        }
    }

    m_running = true;
}

void Physics::stop() {
    std::lock_guard<std::recursive_mutex> world(m_worldMutex);

    if (!m_running)
        return;

    m_running = false;

    const btCollisionObjectArray& objects =
        m_dynamicsWorld->getCollisionObjectArray();
    for (int i = 0; i < objects.size(); i++) {
        btRigidBody* body = btRigidBody::upcast(objects[i]);
        if (!body)
            continue;

        auto it = m_savedStates.find(body);
        if (it == m_savedStates.end())
            continue;

        const BodyState& saved = it->second;

        body->setWorldTransform(saved.transform);
        body->setInterpolationWorldTransform(saved.transform);
        if (body->getMotionState())
            body->getMotionState()->setWorldTransform(saved.transform);

        body->clearForces();
        body->setLinearVelocity(saved.linearVelocity);
        body->setAngularVelocity(saved.angularVelocity);
        body->setInterpolationLinearVelocity(saved.linearVelocity);
        body->setInterpolationAngularVelocity(saved.angularVelocity);

        m_dynamicsWorld->updateSingleAabb(body);
        body->activate(true);
    }

    m_savedStates.clear();

    std::lock_guard<std::mutex> lock(m_networkTargetsMutex);
    m_networkTargets.clear();
}

void Physics::step(float deltaTime) {
    if (!m_running)
        return;

    if (deltaTime > 0.1f)
        deltaTime = 0.1f;

    {
        std::lock_guard<std::recursive_mutex> world(m_worldMutex);
        m_dynamicsWorld->stepSimulation(deltaTime, 5, 1.0f / 60.0f);
    }

    std::vector<std::shared_ptr<types::Instance>> fallen;
    {
        std::lock_guard<std::mutex> lock(m_bodiesMutex);
        for (const auto& [networkId, body] : m_bodies) {
            if (!body)
                continue;

            auto* motionState =
                static_cast<BodyMotionState*>(body->getMotionState());
            if (!motionState || !motionState->isPendingDestroy())
                continue;

            if (types::Instance* target = motionState->getTarget())
                fallen.push_back(target->shared_from_this());

            motionState->detach();
        }
    }

    for (auto& instance : fallen) {
        instance->destroy();
    }
}

void Physics::stepNetworkInterpolation(float dt) {
    std::lock_guard<std::recursive_mutex> world(m_worldMutex);
    std::lock_guard<std::mutex> lock(m_networkTargetsMutex);

    if (m_networkTargets.empty())
        return;

    float alpha = 1.0f - glm::exp(-kNetworkInterpolationSpeed * dt);

    for (auto& [id, target] : m_networkTargets) {
        auto it = m_bodies.find(id);
        if (it == m_bodies.end() || !it->second)
            continue;

        btRigidBody* body = it->second;

        if (body->getCollisionFlags() &
            btCollisionObject::CF_KINEMATIC_OBJECT) {
            const btTransform& transform = body->getWorldTransform();
            const btVector3& currentOrigin = transform.getOrigin();
            btQuaternion currentRotation = transform.getRotation();

            glm::vec3 currentPosition(
                currentOrigin.x(), currentOrigin.y(), currentOrigin.z()
            );
            glm::quat currentQuat(
                currentRotation.w(), currentRotation.x(), currentRotation.y(),
                currentRotation.z()
            );

            glm::vec3 newPosition =
                glm::mix(currentPosition, target.position, alpha);
            glm::quat newRotation =
                glm::slerp(currentQuat, target.rotation, alpha);

            btTransform newTransform;
            newTransform.setOrigin(
                btVector3(newPosition.x, newPosition.y, newPosition.z)
            );
            newTransform.setRotation(btQuaternion(
                newRotation.x, newRotation.y, newRotation.z, newRotation.w
            ));

            body->setWorldTransform(newTransform);
            if (body->getMotionState()) {
                body->getMotionState()->setWorldTransform(newTransform);
            }
        }

        if (!(body->getCollisionFlags() &
              btCollisionObject::CF_KINEMATIC_OBJECT)) {
            body->activate(true);
        }
    }
}

} // namespace game::physics