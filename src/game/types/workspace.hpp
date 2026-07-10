#pragma once

#include "instance.hpp"

namespace game::types {

class Workspace : public Instance {
public:
    Workspace() : Instance(enums::InstanceType::Workspace, "Workspace") {}

    float getGravity() const { return m_gravity; }
    void setGravity(float gravity) {
        m_gravity = gravity;
        propertyChanged();
    }

private:
    float m_gravity = -196.2f;
};

} // namespace game::types