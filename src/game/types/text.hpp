#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>

namespace game::types {

class Text : public TypedInstance<Text> {
public:
    Text(const std::string& name = "Text") : TypedInstance<Text>(enums::InstanceType::Text, name) {}

    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<Text> self = std::static_pointer_cast<Text>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> textProps = {
            {
                "Position",
                "Text 2D position",
                enums::PropertyType::Vector2,
                static_cast<void*>(&m_position),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            },
            {
                "Scale",
                "Text 2D scale",
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
                "Text content",
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
                "Text font",
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
            std::make_move_iterator(textProps.begin()),
            std::make_move_iterator(textProps.end()));

        return instanceProps;
    }

    glm::vec2 getPosition() const { return m_position; }
    void setPosition(const glm::vec2& position) {
        m_position = position;
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
    glm::vec2 m_position{0.0f};
    glm::vec2 m_scale{0.5f};

    std::string m_text = "Text";

    enums::TextFont m_textFont = enums::TextFont::Nunito;
};

} // namespace game::types