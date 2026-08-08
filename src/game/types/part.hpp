#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <btBulletDynamicsCommon.h>

namespace game::types {

class Part : public Instance {
public:
    Part(const std::string& name = "Part") : Instance(enums::InstanceType::Part, name) {
        updateModelMatrix();
    }

    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position, bool silent = false) {
        m_position = position;
        updateModelMatrix();

        if (!silent)
            propertyChanged();
    }

    glm::quat getOrientation() const { return m_orientation; }
    void setOrientation(const glm::quat& orientation, bool silent = false) {
        m_orientation = orientation;
        updateModelMatrix();

        if (!silent)
            propertyChanged();
    }

    glm::vec3 getSize() const { return m_size; }
    void setSize(const glm::vec3& size) {
        if (m_shape == enums::PartType::Cylinder) {
            float diameter = size.x;

            if (glm::abs(size.z - m_size.z) > 0.001f)
                diameter = size.z;

            m_size = glm::vec3(diameter, size.y, diameter);
        } else {
            m_size = size;
        }
        
        updateModelMatrix();
        propertyChanged();
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
    void setColor(const glm::u8vec3& color) {
        m_color = color;
        propertyChanged();
    }

    float getTransparency() const { return m_transparency; }
    void setTransparency(float transparency) {
        m_transparency = std::clamp(transparency, 0.0f, 1.0f);
        propertyChanged();
    }

    bool getAnchored() const { return m_anchored; }
    void setAnchored(bool anchored) {
        m_anchored = anchored;
        propertyChanged();
    }

    enums::PartType getShape() const { return m_shape; }
    void setShape(enums::PartType shape) {
        m_shape = shape;
        propertyChanged();
    }

    enums::MaterialType getMaterial() const { return m_material; }
    void setMaterial(enums::MaterialType material) {
        m_material = material;
        propertyChanged();
    }

    enums::SurfaceType getSurfaceTop() const { return m_surfaceTop; }
    void setSurfaceTop(enums::SurfaceType surface) {
        m_surfaceTop = surface;
        propertyChanged();
    }

    enums::SurfaceType getSurfaceBottom() const { return m_surfaceBottom; }
    void setSurfaceBottom(enums::SurfaceType surface) {
        m_surfaceBottom = surface;
        propertyChanged();
    }

    enums::SurfaceType getSurfaceLeft() const { return m_surfaceLeft; }
    void setSurfaceLeft(enums::SurfaceType surface) {
        m_surfaceLeft = surface;
        propertyChanged();
    }

    enums::SurfaceType getSurfaceRight() const { return m_surfaceRight; }
    void setSurfaceRight(enums::SurfaceType surface) {
        m_surfaceRight = surface;
        propertyChanged();
    }

    enums::SurfaceType getSurfaceFront() const { return m_surfaceFront; }
    void setSurfaceFront(enums::SurfaceType surface) {
        m_surfaceFront = surface;
        propertyChanged();
    }

    enums::SurfaceType getSurfaceBack() const { return m_surfaceBack; }
    void setSurfaceBack(enums::SurfaceType surface) {
        m_surfaceBack = surface;
        propertyChanged();
    }

    btRigidBody* getRigidBody() const { return m_rigidBody; }
    void setRigidBody(btRigidBody* rigidBody) { m_rigidBody = rigidBody; }

private:
    glm::vec3 m_position{0.0f};
    glm::quat m_orientation{0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 m_size{4.0f, 1.0f, 2.0f};

    glm::mat4 m_modelMatrix{1.0f};

    glm::u8vec3 m_color{160, 165, 169}; // medium stone grey
    float m_transparency = 0.0f;

    bool m_anchored = false;

    enums::PartType m_shape = enums::PartType::Block;

    enums::MaterialType m_material = enums::MaterialType::SmoothPlastic;

    enums::SurfaceType m_surfaceTop = enums::SurfaceType::Studs;
    enums::SurfaceType m_surfaceBottom = enums::SurfaceType::Inlets;
    enums::SurfaceType m_surfaceLeft = enums::SurfaceType::Smooth;
    enums::SurfaceType m_surfaceRight = enums::SurfaceType::Smooth;
    enums::SurfaceType m_surfaceFront = enums::SurfaceType::Smooth;
    enums::SurfaceType m_surfaceBack = enums::SurfaceType::Smooth;

    btRigidBody* m_rigidBody = nullptr;
};

} // namespace game::types