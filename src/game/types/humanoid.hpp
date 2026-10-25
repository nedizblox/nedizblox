#pragma once

#include "instance.hpp"

namespace game::types {

class Humanoid : public TypedInstance<Humanoid> {
public:
    Humanoid(const std::string& name = "Humanoid") :
        TypedInstance<Humanoid>(enums::InstanceType::Humanoid, name) {
    }

    static std::shared_ptr<Humanoid>
    create(const std::string& name = "Humanoid") {
        return std::make_shared<Humanoid>(name);
    }

    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<Humanoid> self =
            std::static_pointer_cast<Humanoid>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> humanoidProps = {
            {"WalkSpeed", "Move speed for the rig", enums::PropertyType::Float,
             static_cast<void*>(&m_walkSpeed), false,
             [self]()
             {
                 if (auto instance = self.lock())
                     instance->propertyChanged();
             }},
            {
                "JumpPower",
                "Jump power for the rig",
                enums::PropertyType::Float,
                static_cast<void*>(&m_jumpPower),
                false,
                [self]()
                {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                },
            },
            {"Health", "Health of the rig", enums::PropertyType::UInt32,
             static_cast<void*>(&m_health), false,
             [self]()
             {
                 if (auto instance = self.lock())
                     instance->propertyChanged();
             }},
            {"MaxHealth", "Maximum health of the rig",
             enums::PropertyType::UInt32, static_cast<void*>(&m_maxHealth),
             false, [self]()
             {
                 if (auto instance = self.lock())
                     instance->propertyChanged();
             }}
        };

        instanceProps.insert(
            instanceProps.end(), std::make_move_iterator(humanoidProps.begin()),
            std::make_move_iterator(humanoidProps.end())
        );

        return instanceProps;
    }

    float getWalkSpeed() const {
        return m_walkSpeed;
    }
    void setWalkSpeed(float walkSpeed) {
        m_walkSpeed = walkSpeed;
    }

    float getJumpPower() const {
        return m_jumpPower;
    }
    void setJumpPower(float jumpPower) {
        m_jumpPower = jumpPower;
    }

    uint32_t getHealth() const {
        return m_health;
    }
    uint32_t getMaxHealth() const {
        return m_maxHealth;
    }

    void setHealth(uint32_t health) {
        m_health = glm::min(health, m_maxHealth);
    }
    void setMaxHealth(uint32_t maxHealth) {
        m_maxHealth = maxHealth;
        m_health = glm::min(m_health, m_maxHealth);
    }

    void takeDamage(uint32_t amount) {
        m_health = (amount >= m_health) ? 0 : m_health - amount;
    }

    bool isDead() const {
        return m_health == 0;
    }

    void serialize(net::serializer::BinaryWriter& writer) const override {
        Instance::serialize(writer);

        writer.write<float>(m_walkSpeed);
        writer.write<float>(m_jumpPower);

        writer.write<uint32_t>(m_health);
        writer.write<uint32_t>(m_maxHealth);
    }

    void deserialize(net::serializer::BinaryReader& reader) override {
        Instance::deserialize(reader);

        m_walkSpeed = reader.read<float>();
        m_jumpPower = reader.read<float>();

        m_health = reader.read<uint32_t>();
        m_maxHealth = reader.read<uint32_t>();
    }

private:
    float m_walkSpeed = 16.0f;
    float m_jumpPower = 50.0f;

    uint32_t m_health = 100;
    uint32_t m_maxHealth = 100;
};

} // namespace game::types