#include "rig.hpp"

namespace game::prefabs {

Rig::Rig(physics::Physics& physics, const std::string& name) :
    m_physics(physics), types::Model(name) {
    createBodyParts();
}

void Rig::createBodyParts() {
    m_head = std::make_shared<types::Part>("Head");
    auto torso = std::make_shared<types::Part>("Torso");
    auto rootPart = std::make_shared<types::Part>("RootPart");
    auto leftArm = std::make_shared<types::Part>("LeftArm");
    auto rightArm = std::make_shared<types::Part>("RightArm");
    auto leftLeg = std::make_shared<types::Part>("LeftLeg");
    auto rightLeg = std::make_shared<types::Part>("RightLeg");

    m_head->setSize(glm::vec3(1.0f, 1.0f, 1.0f));
    torso->setSize(glm::vec3(2.0f, 2.0f, 1.0f));
    rootPart->setSize(glm::vec3(2.0f, 6.0f, 1.0f));
    leftArm->setSize(glm::vec3(1.0f, 2.0f, 1.0f));
    rightArm->setSize(glm::vec3(1.0f, 2.0f, 1.0f));
    leftLeg->setSize(glm::vec3(1.0f, 2.0f, 1.0f));
    rightLeg->setSize(glm::vec3(1.0f, 2.0f, 1.0f));

    m_head->setColor(glm::u8vec3(255, 255, 0));
    torso->setColor(glm::u8vec3(0, 13, 255));
    leftArm->setColor(glm::u8vec3(255, 255, 0));
    rightArm->setColor(glm::u8vec3(255, 255, 0));
    leftLeg->setColor(glm::u8vec3(0, 98, 255));
    rightLeg->setColor(glm::u8vec3(0, 98, 255));

    rootPart->setTransparency(1.0f);

    m_head->setShape(enums::PartType::Head);

    m_rigidBody = m_physics.createRigidBodyModel(this, rootPart.get());
    m_rigidBody->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));
    m_rigidBody->setActivationState(DISABLE_DEACTIVATION);
    m_rigidBody->setFriction(0.1f);

    glm::quat identityRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    m_bones.push_back({m_head, glm::vec3(0.0f, 1.5f, 0.0f), identityRot});
    m_bones.push_back({torso, glm::vec3(0.0f, 0.0f, 0.0f), identityRot});
    m_bones.push_back({leftArm, glm::vec3(-1.5f, 0.0f, 0.0f), identityRot});
    m_bones.push_back({rightArm, glm::vec3(1.5f, 0.0f, 0.0f), identityRot});
    m_bones.push_back({leftLeg, glm::vec3(-0.5f, -2.0f, 0.0f), identityRot});
    m_bones.push_back({rightLeg, glm::vec3(0.5f, -2.0f, 0.0f), identityRot});

    rootPart->setParent(this);

    for (auto& bone : m_bones) {
        bone.part->setParent(this);
    }

    m_nickname = std::make_shared<types::BillboardText>();
    m_nickname->setText(getName());
    m_nickname->setOffset(glm::vec3(0.0f, 2.3f, 0.0f));
    m_nickname->setPosition(getPivotPosition());
    m_nickname->setParent(m_head.get());

    syncParts();

    setPivotPosition(m_spawnPosition);
}

void Rig::syncParts() {
    glm::mat4 pivot = getPivot();
    glm::quat modelOrientation = glm::toQuat(pivot);

    for (const auto& bone : m_bones) {
        glm::vec4 worldPos = pivot * glm::vec4(bone.localPosition, 1.0f);

        bone.part->setPosition(glm::vec3(worldPos));
        bone.part->setOrientation(modelOrientation * bone.localOrientation);
    }
}

void Rig::setPivotPosition(const glm::vec3& position) {
    types::Model::setPivotPosition(position);

    if (m_rigidBody) {
        btTransform trans = m_rigidBody->getWorldTransform();
        trans.setOrigin(btVector3(position.x, position.y, position.z));
        m_rigidBody->setWorldTransform(trans);

        m_rigidBody->activate(true);
    }
}

void Rig::move(MoveDirection direction, float phi) {
    glm::vec3 forward = glm::normalize(glm::vec3(-glm::sin(phi), 0.0f, -glm::cos(phi)));

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    glm::vec3 moveDir(0.0f);
    if (direction == MoveDirection::Forward)
        moveDir += forward;
    if (direction == MoveDirection::Backward)
        moveDir -= forward;
    if (direction == MoveDirection::Left)
        moveDir -= right;
    if (direction == MoveDirection::Right)
        moveDir += right;

    if (glm::length(moveDir) > 0.0f) {
        m_velocity += glm::normalize(moveDir);
    }
}

void Rig::jump() {
    if (!m_rigidBody || !isGrounded())
        return;

    btVector3 currentVelocity = m_rigidBody->getLinearVelocity();
    m_rigidBody->setLinearVelocity(btVector3(currentVelocity.x(), m_jumpPower, currentVelocity.z()));

    m_rigidBody->activate(true);
}

bool Rig::isGrounded() {
    if (!m_rigidBody)
        return false;

    btTransform trans = m_rigidBody->getWorldTransform();
    btVector3 start = trans.getOrigin();

    btVector3 end = start + btVector3(0.0f, -3.1f, 0.0f);

    btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);

    m_physics.getDynamicsWorld()->rayTest(start, end, rayCallback);

    if (rayCallback.hasHit()) {
        if (rayCallback.m_collisionObject != m_rigidBody) {
            return true;
        }
    }

    return false;
}

void Rig::update(float deltaTime) {
    if (!m_rigidBody)
        return;

    btVector3 currentVelocity = m_rigidBody->getLinearVelocity();
    btTransform trans = m_rigidBody->getWorldTransform();

    if (glm::length(m_velocity) > 0.01f) {
        glm::vec3 finalVelocity = glm::normalize(m_velocity) * m_walkSpeed;
        
        m_rigidBody->setLinearVelocity(btVector3(finalVelocity.x, currentVelocity.y(), finalVelocity.z));

        float targetAngle = std::atan2(m_velocity.x, m_velocity.z);
        btQuaternion targetRotation(btVector3(0.0f, 1.0f, 0.0f), targetAngle);

        btQuaternion currentRotation = trans.getRotation();

        float interpFactor = 10.0f * deltaTime;
        if (interpFactor > 1.0f)
            interpFactor = 1.0f;

        btQuaternion newRotation = currentRotation.slerp(targetRotation, interpFactor);

        trans.setRotation(newRotation);
        m_rigidBody->setWorldTransform(trans);

        m_rigidBody->activate(true);
    } else {
        m_rigidBody->setLinearVelocity(btVector3(0.0f, currentVelocity.y(), 0.0f));
    }

    btVector3 origin = trans.getOrigin();
    types::Model::setPivotPosition(glm::vec3(origin.x(), origin.y(), origin.z()));

    m_velocity = glm::vec3(0.0f);

    m_nickname->setPosition(getPivotPosition());
    m_nickname->setText(getName());
}

} // namespace game::prefabs