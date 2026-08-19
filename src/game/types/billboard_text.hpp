#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>

namespace game::types {

class BillboardText : public TypedInstance<BillboardText> {
public:
    BillboardText(const std::string& name = "BillboardText") :
        TypedInstance<BillboardText>(enums::InstanceType::BillboardText, name) {}

    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<BillboardText> self = std::static_pointer_cast<BillboardText>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> billbTextProps = {
            {
                "Offset",
                "Billboard Text 3D offset of position",
                enums::PropertyType::Vector3,
                static_cast<void*>(&m_offset),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            },
            {
                "Scale",
                "Billboard Text 2D scale",
                enums::PropertyType::Vector2,
                static_cast<void*>(&m_scale),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            },
            {
                "Text",
                "Billboard Text content",
                enums::PropertyType::String,
                static_cast<void*>(&m_text),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            },
            {
                "TextFont",
                "Billboard Text font",
                enums::PropertyType::Enum,
                static_cast<void*>(&m_textFont),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                },
                nullptr,
                nullptr,
                enums::getStringsFromEnums<enums::TextFont>()
            }
        };

        instanceProps.insert(
            instanceProps.end(),
            std::make_move_iterator(billbTextProps.begin()),
            std::make_move_iterator(billbTextProps.end()));

        return instanceProps;
    }

    glm::vec3 getOffset() const { return m_offset; }
    void setOffset(const glm::vec3& offset) {
        m_offset = offset;
        propertyChanged();
    }

    glm::vec2 getScale() const { return m_scale; }
    void setScale(const glm::vec2& scale) {
        m_scale = scale;
        propertyChanged();
    }

    std::string getText() const { return m_text; }
    void setText(const std::string& text) {
        m_text = text;
        propertyChanged();
    }

    enums::TextFont getTextFont() const { return m_textFont; }
    void setTextFont(enums::TextFont font) { m_textFont = font; }

private:
    glm::vec3 m_offset{0.0f};
    glm::vec2 m_scale{0.009f};

    std::string m_text = "Billboard Text";

    enums::TextFont m_textFont = enums::TextFont::Nunito;
};

} // namespace game::types