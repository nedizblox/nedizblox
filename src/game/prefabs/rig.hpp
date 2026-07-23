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

    static std::shared_ptr<Rig>
    create(game::physics::Physics& physics, const std::string& name = "Rig", uint32_t networkId = 0);

    std::shared_ptr<types::Part> getHead() const { return m_head; }

    btRigidBody* getRigidBody() const { return m_rigidBody; }

    float getWalkSpeed() const { return m_walkSpeed; }
    void setWalkSpeed(float walkSpeed) { m_walkSpeed = walkSpeed; }

    glm::vec3 getSpawnPosition() const { return m_spawnPosition; }
    void setSpawnPosition(const glm::vec3& spawnPosition) { m_spawnPosition = spawnPosition; }

    void setPivotPosition(const glm::vec3& position) override;

    void move(enums::RigMoveDirection direction, float phi);
    void jump();

    bool isGrounded();

    void update(float deltaTime);

private:
    Rig(physics::Physics& physics, const std::string& name, uint32_t networkId) :
        m_physics(physics), types::Model(name), m_networkId(networkId) {}

    physics::Physics& m_physics;

    uint32_t m_networkId = 0;

    std::vector<Bone> m_bones;

    std::shared_ptr<types::Part> m_head;

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