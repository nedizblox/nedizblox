#pragma once

#include "game/enums/enums.hpp"
#include "instance.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace game::types {

class Part : public Instance {
public:
    Part(const std::string& name = "Part") : Instance(enums::InstanceType::Part, name) {
        updateModelMatrix();
    }

    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) {
        m_position = position;
        updateModelMatrix();
    }

    glm::quat getOrientation() const { return m_orientation; }
    void setOrientation(const glm::quat& orientation) {
        m_orientation = orientation;
        updateModelMatrix();
    }

    glm::vec3 getSize() const { return m_size; }
    void setSize(const glm::vec3& size) {
        m_size = size;
        updateModelMatrix();
    }

    glm::mat4 getModelMatrix() const { return m_modelMatrix; }
    void updateModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, m_position);
        model *= glm::mat4_cast(m_orientation);
        model = glm::scale(model, m_size);

        m_modelMatrix = model;
    }

    glm::u8vec3 getColor() const { return m_color; }
    void setColor(const glm::u8vec3& color) { m_color = color; }

    float getTransparency() const { return m_transparency; }
    void setTransparency(float transparency) {
        m_transparency = std::clamp(transparency, 0.0f, 1.0f);
    }

    bool getAnchored() const { return m_anchored; }
    void setAnchored(bool anchored) { m_anchored = anchored; }

    enums::PartType getShape() const { return m_shape; }
    void setShape(enums::PartType shape) { m_shape = shape; }

private:
    glm::vec3 m_position{0.0f};
    glm::quat m_orientation;
    glm::vec3 m_size{4.0f, 1.0f, 2.0f};

    glm::mat4 m_modelMatrix{1.0f};

    glm::u8vec3 m_color{160, 165, 169}; // medium stone grey
    float m_transparency = 0.0f;

    bool m_anchored = false;

    enums::PartType m_shape = enums::PartType::Block;
};

} // namespace game::types