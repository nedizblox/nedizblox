#pragma once

#include "instance.hpp"

namespace game::types {

class Workspace : public TypedInstance<Workspace> {
public:
    Workspace() : TypedInstance<Workspace>(enums::InstanceType::Workspace, "Workspace") {}
    
    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<Workspace> self = std::static_pointer_cast<Workspace>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> workspaceProps = {
            {
                "Gravity",
                "Physics world gravity",
                enums::PropertyType::Float,
                static_cast<void*>(&m_gravity),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            }
        };

        instanceProps.insert(
            instanceProps.end(),
            std::make_move_iterator(workspaceProps.begin()),
            std::make_move_iterator(workspaceProps.end()));

        return instanceProps;
    }

    float getGravity() const { return m_gravity; }
    void setGravity(float gravity) {
        m_gravity = gravity;
        propertyChanged();
    }

private:
    float m_gravity = -196.2f;
};

} // namespace game::types