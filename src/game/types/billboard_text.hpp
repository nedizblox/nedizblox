#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>

namespace game::types {

class BillboardText : public Instance {
public:
    BillboardText(const std::string& name = "BillboardText") : Instance(enums::InstanceType::BillboardText, name) {}

    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }

    glm::vec3 getOffset() const { return m_offset; }
    void setOffset(const glm::vec3& offset) { m_offset = offset; }

    glm::vec2 getScale() const { return m_scale; }
    void setScale(const glm::vec2& scale) { m_scale = scale; }

    std::string getText() const { return m_text; }
    void setText(const std::string& text) { m_text = text; }

private:
    glm::vec3 m_position{0.0f};
    glm::vec3 m_offset{0.0f};
    glm::vec2 m_scale{0.009f};

    std::string m_text = "Billboard Text";
};

} // namespace game::types