#include "humanoid_manager.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>

namespace {

struct IgnoreSelfConvexCallback
    : public btCollisionWorld::ClosestConvexResultCallback {
    IgnoreSelfConvexCallback(
        const btVector3& from, const btVector3& to,
        const btCollisionObject* self
    ) : btCollisionWorld::ClosestConvexResultCallback(from, to), m_self(self) {
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override {
        if (proxy0->m_clientObject == m_self)
            return false;
        return btCollisionWorld::ClosestConvexResultCallback::needsCollision(
            proxy0
        );
    }

    const btCollisionObject* m_self;
};

bool isSpawnLocation(const std::shared_ptr<game::types::Instance>& instance) {
    if (dynamic_cast<game::types::SpawnLocation*>(instance.get()))
        return true;

    auto* part = dynamic_cast<game::types::Part*>(instance.get());
    return part && part->getPartType() == game::enums::PartType::SpawnLocation;
}

void collectSpawnLocations(
    const std::shared_ptr<game::types::Instance>& node,
    std::vector<std::shared_ptr<game::types::Part>>& out
) {
    if (!node)
        return;

    for (const auto& child : node->getChildren()) {
        if (!child || dynamic_cast<game::types::Rig*>(child.get()))
            continue;

        if (isSpawnLocation(child))
            out.push_back(std::static_pointer_cast<game::types::Part>(child));

        collectSpawnLocations(child, out);
    }
}

} // namespace

namespace game::mngrs {

HumanoidManager::HumanoidManager(
    physics::Physics& physics, std::shared_ptr<types::Workspace> workspace
) :
    m_physics(physics), m_workspace(std::move(workspace)),
    m_rng(std::random_device{}()) {
    watchWorkspace();
}

HumanoidManager::~HumanoidManager() {
    m_aliveToken.reset();
}

void HumanoidManager::watchWorkspace() {
    std::weak_ptr<bool> alive = m_aliveToken;

    m_workspace->onDescendantAdded(
        [this, alive](std::shared_ptr<types::Instance> added)
        {
            if (alive.expired())
                return;
            onInstanceAdded(added);
        }
    );

    m_workspace->onDescendantDestroy(
        [this, alive](std::shared_ptr<types::Instance> destroyed)
        {
            if (alive.expired())
                return;
            onInstanceDestroyed(destroyed);
        }
    );

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& descendant : m_workspace->getDescendants()) {
        if (auto rig = std::dynamic_pointer_cast<types::Rig>(descendant))
            queueRig(rig);
    }
}

std::shared_ptr<types::Rig> HumanoidManager::createHumanoid(
    const std::string& name, uint32_t networkId, bool isLocal,
    const glm::vec3& spawnPosition
) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (auto it = m_states.find(networkId); it != m_states.end())
        return it->second.rig;

    auto rig = types::Rig::create(name);
    rig->setNetworkId(networkId);

    State state;
    state.rig = rig;
    state.rootPart = rig->getRootPart();
    state.humanoid = rig->getHumanoid();
    state.spawnPosition = spawnPosition;
    state.isLocal = isLocal;

    if (!state.rootPart)
        return nullptr;

    state.rootPart->setNetworkId(networkId);

    for (const auto& layout : types::Rig::getLimbLayout()) {
        if (auto part = rig->getPart(layout.name))
            state.limbs.push_back({part, layout.offset});
    }

    if (isLocal) {
        glm::vec3 picked;
        if (findSpawnPosition(state, picked))
            state.spawnPosition = picked;
        else
            state.awaitingSpawn = true;
    }

    rig->setPivotPosition(state.spawnPosition);
    syncParts(state);

    createBody(state);

    m_states.emplace(networkId, std::move(state));

    rig->setParent(m_workspace);

    return rig;
}

void HumanoidManager::removeHumanoid(uint32_t networkId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    if (it == m_states.end())
        return;

    State state = std::move(it->second);
    m_states.erase(it);

    m_physics.destroyRigidBody(state.rootPart.get());
    for (auto& limb : state.limbs)
        m_physics.destroyRigidBody(limb.part.get());

    state.rig->destroy();
}

bool HumanoidManager::hasHumanoid(uint32_t networkId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_states.find(networkId) != m_states.end();
}

std::shared_ptr<types::Rig> HumanoidManager::getRig(uint32_t networkId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    return it != m_states.end() ? it->second.rig : nullptr;
}

btRigidBody* HumanoidManager::getRigidBody(uint32_t networkId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    return it != m_states.end() ? it->second.rig->getRigidBody() : nullptr;
}

bool HumanoidManager::isDead(uint32_t networkId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    return it != m_states.end() && it->second.dead;
}

void HumanoidManager::move(
    uint32_t networkId, const glm::vec2& axis, float phi
) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    if (it == m_states.end())
        return;

    State& state = it->second;
    if (state.dead || !state.isLocal)
        return;

    glm::vec2 input = axis;
    float length = glm::length(input);
    if (length > 1.0f)
        input /= length;

    glm::vec3 forward(-glm::sin(phi), 0.0f, -glm::cos(phi));
    glm::vec3 right(glm::cos(phi), 0.0f, -glm::sin(phi));

    state.moveDir = forward * input.y + right * input.x;
}

bool HumanoidManager::jump(uint32_t networkId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    if (it == m_states.end())
        return false;

    State& state = it->second;
    if (state.dead || !state.isLocal)
        return false;

    btRigidBody* body = state.rig->getRigidBody();
    if (!body || !isGrounded(state, body))
        return false;

    float jumpPower = state.humanoid ? state.humanoid->getJumpPower() : 50.0f;

    btVector3 velocity = body->getLinearVelocity();
    body->setLinearVelocity(btVector3(velocity.x(), jumpPower, velocity.z()));
    body->activate(true);

    return true;
}

void HumanoidManager::damage(uint32_t networkId, uint32_t amount) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    if (it == m_states.end() || it->second.dead || !it->second.humanoid)
        return;

    it->second.humanoid->takeDamage(amount);
    if (it->second.humanoid->isDead())
        killState(it->second);
}

void HumanoidManager::kill(uint32_t networkId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_states.find(networkId);
    if (it == m_states.end())
        return;

    if (it->second.humanoid)
        it->second.humanoid->setHealth(0);

    killState(it->second);
}

void HumanoidManager::update(float dt) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    retryPendingRigs();

    for (auto& [networkId, state] : m_states) {
        if (state.dead) {
            state.respawnTimer -= dt;
            if (state.respawnTimer <= 0.0f)
                respawnState(state);
            continue;
        }

        btRigidBody* body = state.rig->getRigidBody();
        if (!body)
            continue;

        if (state.isLocal) {
            if (state.humanoid && state.humanoid->isDead()) {
                killState(state);
                continue;
            }

            if (state.awaitingSpawn) {
                state.spawnRetryTimer -= dt;
                if (state.spawnRetryTimer <= 0.0f) {
                    state.spawnRetryTimer = kSpawnRetryInterval;

                    glm::vec3 picked;
                    if (findSpawnPosition(state, picked)) {
                        state.spawnPosition = picked;
                        state.awaitingSpawn = false;
                        teleportToSpawn(state, body);
                        continue;
                    }
                }
            }

            updateLocalMovement(state, body, dt);
        }

        syncParts(state);
    }
}

void HumanoidManager::createBody(State& state) {
    btRigidBody* body = m_physics.createRigidBodyRig(state.rig.get());
    if (!body)
        return;

    m_physics.setBodyNetworkMode(body, state.isLocal);
}

void HumanoidManager::syncParts(State& state) {
    glm::mat4 pivot = state.rig->getPivot();
    glm::quat orientation = glm::toQuat(pivot);

    state.rootPart->setPosition(glm::vec3(pivot[3]), true);
    state.rootPart->setOrientation(orientation, true);

    for (const auto& limb : state.limbs) {
        glm::vec3 worldPos = glm::vec3(pivot * glm::vec4(limb.offset, 1.0f));

        limb.part->setPosition(worldPos, true);
        limb.part->setOrientation(orientation, true);
    }
}

void HumanoidManager::updateLocalMovement(
    State& state, btRigidBody* body, float dt
) {
    btTransform transform = body->getWorldTransform();

    if (transform.getOrigin().y() < kRespawnHeight) {
        teleportToSpawn(state, body);
        state.moveDir = glm::vec3(0.0f);
        return;
    }

    float walkSpeed = state.humanoid ? state.humanoid->getWalkSpeed() : 16.0f;
    btVector3 velocity = body->getLinearVelocity();

    if (glm::length(state.moveDir) > kInputDeadzone) {
        glm::vec3 target = state.moveDir * walkSpeed;

        body->setLinearVelocity(btVector3(target.x, velocity.y(), target.z));

        float targetAngle = std::atan2(state.moveDir.x, state.moveDir.z);
        btQuaternion targetRotation(btVector3(0.0f, 1.0f, 0.0f), targetAngle);

        float interpolation = glm::min(kTurnSpeed * dt, 1.0f);

        transform.setRotation(
            transform.getRotation().slerp(targetRotation, interpolation)
        );
        body->setWorldTransform(transform);
    } else {
        body->setLinearVelocity(btVector3(0.0f, velocity.y(), 0.0f));
    }

    body->activate(true);

    state.moveDir = glm::vec3(0.0f);
}

bool HumanoidManager::isGrounded(const State& state, btRigidBody* body) {
    std::lock_guard<std::recursive_mutex> world(m_physics.getWorldMutex());

    btTransform transform = body->getWorldTransform();
    btVector3 start = transform.getOrigin();

    float halfHeight = state.rootPart->getSize().y * 0.5f;
    float probeDistance =
        halfHeight - kGroundProbeRadius + kGroundProbeTolerance;

    btSphereShape castShape(kGroundProbeRadius);

    btTransform castStart = transform;
    btTransform castEnd = transform;
    castEnd.setOrigin(start + btVector3(0.0f, -probeDistance, 0.0f));

    IgnoreSelfConvexCallback callback(start, castEnd.getOrigin(), body);

    if (body->getBroadphaseProxy()) {
        callback.m_collisionFilterGroup =
            body->getBroadphaseProxy()->m_collisionFilterGroup;
        callback.m_collisionFilterMask =
            body->getBroadphaseProxy()->m_collisionFilterMask;
    }

    m_physics.getDynamicsWorld()->convexSweepTest(
        &castShape, castStart, castEnd, callback
    );

    return callback.hasHit() && callback.m_hitNormalWorld.y() >= 0.5f;
}

void HumanoidManager::teleportToSpawn(State& state, btRigidBody* body) {
    glm::vec3 picked;
    if (findSpawnPosition(state, picked))
        state.spawnPosition = picked;

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        state.spawnPosition.x, state.spawnPosition.y, state.spawnPosition.z
    ));

    body->setWorldTransform(transform);
    if (body->getMotionState())
        body->getMotionState()->setWorldTransform(transform);

    body->clearForces();
    body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
    body->activate(true);

    syncParts(state);
}

void HumanoidManager::killState(State& state) {
    if (state.dead)
        return;

    syncParts(state);

    btVector3 inherited(0.0f, 0.0f, 0.0f);
    if (btRigidBody* body = state.rig->getRigidBody())
        inherited = body->getLinearVelocity();

    m_physics.destroyRigidBody(state.rootPart.get());

    for (auto& limb : state.limbs) {
        btRigidBody* limbBody =
            m_physics.createRigidBodyPart(limb.part.get(), false);
        if (!limbBody)
            continue;

        limbBody->setLinearVelocity(
            inherited +
            btVector3(
                randomRange(-kRagdollScatterSpeed, kRagdollScatterSpeed),
                randomRange(0.0f, kRagdollScatterSpeed),
                randomRange(-kRagdollScatterSpeed, kRagdollScatterSpeed)
            )
        );
        limbBody->setAngularVelocity(btVector3(
            randomRange(-kRagdollSpinSpeed, kRagdollSpinSpeed),
            randomRange(-kRagdollSpinSpeed, kRagdollSpinSpeed),
            randomRange(-kRagdollSpinSpeed, kRagdollSpinSpeed)
        ));
        limbBody->activate(true);
    }

    state.dead = true;
    state.respawnTimer = kRespawnDelay;
    state.moveDir = glm::vec3(0.0f);
}

void HumanoidManager::respawnState(State& state) {
    for (auto& limb : state.limbs)
        m_physics.destroyRigidBody(limb.part.get());

    glm::vec3 picked;
    if (findSpawnPosition(state, picked))
        state.spawnPosition = picked;

    state.rig->setPivot(
        glm::translate(glm::mat4(1.0f), state.spawnPosition), true
    );

    if (state.humanoid)
        state.humanoid->setHealth(state.humanoid->getMaxHealth());

    createBody(state);
    syncParts(state);

    state.dead = false;
    state.respawnTimer = 0.0f;
}

bool HumanoidManager::findSpawnPosition(
    const State& state, glm::vec3& outPosition
) {
    std::vector<std::shared_ptr<types::Part>> spawns;
    collectSpawnLocations(m_workspace, spawns);

    if (spawns.empty())
        return false;

    std::size_t index =
        std::uniform_int_distribution<std::size_t>(0, spawns.size() - 1)(m_rng);
    const auto& spawn = spawns[index];

    glm::vec3 size = spawn->getSize();
    float rigHalfHeight = state.rootPart->getSize().y * 0.5f;

    glm::vec3 local(
        randomRange(-kSpawnAreaFraction, kSpawnAreaFraction) * size.x * 0.5f,
        size.y * 0.5f + rigHalfHeight + kSpawnClearance,
        randomRange(-kSpawnAreaFraction, kSpawnAreaFraction) * size.z * 0.5f
    );

    outPosition = spawn->getPosition() + spawn->getOrientation() * local;
    return true;
}

void HumanoidManager::onInstanceAdded(
    const std::shared_ptr<types::Instance>& instance
) {
    if (!instance)
        return;

    if (auto rig = std::dynamic_pointer_cast<types::Rig>(instance)) {
        queueRig(rig);
    } else if (instance->getType() == enums::InstanceType::Part) {
        if (auto rig =
                std::dynamic_pointer_cast<types::Rig>(instance->getParent()))
            queueRig(rig);
    }

    for (const auto& descendant : instance->getDescendants()) {
        if (auto rig = std::dynamic_pointer_cast<types::Rig>(descendant))
            queueRig(rig);
    }
}

void HumanoidManager::onInstanceDestroyed(
    const std::shared_ptr<types::Instance>& instance
) {
    if (auto rig = std::dynamic_pointer_cast<types::Rig>(instance))
        forgetRig(rig.get());
}

void HumanoidManager::queueRig(const std::shared_ptr<types::Rig>& rig) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!rig || isTracked(rig.get()))
        return;

    if (tryAdoptRig(rig))
        return;

    for (const auto& pending : m_pendingRigs) {
        if (pending.lock() == rig)
            return;
    }
    m_pendingRigs.push_back(rig);
}

void HumanoidManager::retryPendingRigs() {
    for (auto it = m_pendingRigs.begin(); it != m_pendingRigs.end();) {
        auto rig = it->lock();
        if (!rig || isTracked(rig.get()) || tryAdoptRig(rig))
            it = m_pendingRigs.erase(it);
        else
            ++it;
    }
}

bool HumanoidManager::tryAdoptRig(const std::shared_ptr<types::Rig>& rig) {
    if (isTracked(rig.get()))
        return true;

    if (!isRigComplete(*rig))
        return false;

    State state;
    state.rig = rig;
    state.rootPart = rig->getRootPart();
    state.humanoid = rig->getHumanoid();
    state.isLocal = true;
    state.spawnPosition = rig->getPivotPosition();

    for (const auto& layout : types::Rig::getLimbLayout()) {
        if (auto part = rig->getPart(layout.name))
            state.limbs.push_back({part, layout.offset});
    }

    const uint32_t networkId = allocateAdoptedId();
    rig->setNetworkId(networkId);
    state.rootPart->setNetworkId(networkId);

    syncParts(state);
    createBody(state);

    m_states.emplace(networkId, std::move(state));
    return true;
}

void HumanoidManager::forgetRig(const types::Rig* rig) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        if (it->second.rig.get() != rig)
            continue;

        State state = std::move(it->second);
        m_states.erase(it);

        m_physics.destroyRigidBody(state.rootPart.get());
        for (auto& limb : state.limbs)
            m_physics.destroyRigidBody(limb.part.get());
        break;
    }

    m_pendingRigs.erase(
        std::remove_if(
            m_pendingRigs.begin(), m_pendingRigs.end(),
            [rig](const std::weak_ptr<types::Rig>& pending)
            {
                auto locked = pending.lock();
                return !locked || locked.get() == rig;
            }
        ),
        m_pendingRigs.end()
    );
}

bool HumanoidManager::isTracked(const types::Rig* rig) const {
    for (const auto& [networkId, state] : m_states) {
        if (state.rig.get() == rig)
            return true;
    }
    return false;
}

bool HumanoidManager::isRigComplete(types::Rig& rig) {
    if (!rig.getRootPart())
        return false;

    for (const auto& layout : types::Rig::getLimbLayout()) {
        if (!rig.getPart(layout.name))
            return false;
    }
    return true;
}

uint32_t HumanoidManager::allocateAdoptedId() {
    while (m_states.find(m_nextAdoptedId) != m_states.end())
        ++m_nextAdoptedId;
    return m_nextAdoptedId++;
}

float HumanoidManager::randomRange(float minValue, float maxValue) {
    return std::uniform_real_distribution<float>(minValue, maxValue)(m_rng);
}

} // namespace game::mngrs