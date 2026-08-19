#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>

namespace game::types {

class SelectionBox : public TypedInstance<SelectionBox> {
public:
    SelectionBox(const std::string& name = "Outline") : TypedInstance<SelectionBox>(enums::InstanceType::SelectionBox, name) {}

    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<SelectionBox> self = std::static_pointer_cast<SelectionBox>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> selectionBoxProps = {
            {
                "Color",
                "Selection box color",
                enums::PropertyType::Color3,
                static_cast<void*>(&m_color),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            },
            {
                "Transparency",
                "Selection box transparency",
                enums::PropertyType::Float,
                static_cast<void*>(&m_transparency),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            }
        };

        instanceProps.insert(
            instanceProps.end(),
            std::make_move_iterator(selectionBoxProps.begin()),
            std::make_move_iterator(selectionBoxProps.end()));

        return instanceProps;
    }

    glm::u8vec3 getColor() const { return m_color; }
    void setColor(const glm::u8vec3& color) { m_color = color; propertyChanged(); }

    float getTransparency() const { return m_transparency; }
    void setTransparency(float transparency) {
        m_transparency = std::clamp(transparency, 0.0f, 1.0f);
        propertyChanged();
    }

private:
    glm::u8vec3 m_color{0, 180, 255};

    float m_transparency = 0.0f;
};

} // namespace game::types