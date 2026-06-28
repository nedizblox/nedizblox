#pragma once

#include "../types/model.hpp"

#include "../physics/physics.hpp"

namespace game::prefabs {

class Rig : public types::Model {
public:
    struct Bone {
        std::shared_ptr<types::Part> part;
        glm::vec3 localPosition;
        glm::quat localOrientation;
    };

    enum class MoveDirection { Forward, Backward, Left, Right };

    static std::shared_ptr<Rig> create(physics::Physics& physics, const std::string& name = "Rig");

    float getWalkSpeed() const { return m_walkSpeed; }
    void setWalkSpeed(float walkSpeed) { m_walkSpeed = walkSpeed; }

    glm::vec3 getSpawnPosition() const { return m_spawnPosition; }
    void setSpawnPosition(const glm::vec3& spawnPosition) { m_spawnPosition = spawnPosition; }

    void setPivotPosition(const glm::vec3& position) override;

    void move(MoveDirection direction, float phi);
    void jump();

    bool isGrounded();

    void update(float deltaTime);

private:
    Rig(physics::Physics& physics, const std::string& name = "Rig") :
        m_physics(physics), types::Model(name) {}

    physics::Physics& m_physics;

    std::vector<Bone> m_bones;

    glm::vec3 m_velocity{0.0f};

    float m_walkSpeed = 16.0f;
    float m_jumpPower = 50.0f;

    glm::vec3 m_spawnPosition{0.0f, 5.0f, 0.0f};

    std::shared_ptr<types::BillboardText> m_nickname;

    btRigidBody* m_rigidBody = nullptr;

    void createBodyParts();
    void syncParts();
};

} // namespace game::prefabs