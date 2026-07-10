#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>

namespace game::types {

class Text : public Instance {
public:
    Text(const std::string& name = "Text") : Instance(enums::InstanceType::Text, name) {}

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

private:
    glm::vec2 m_position{0.0f};
    glm::vec2 m_scale{0.5f};

    std::string m_text = "Text";
};

} // namespace game::types