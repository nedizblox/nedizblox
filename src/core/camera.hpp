#pragma once

#include <glm/glm.hpp>

namespace core::camera {

class FreeCamera {
public:
    enum class CameraDirection { Forward, Backward, Left, Right };

    float getMovementSpeed() const { return m_movementSpeed; }
    void setMovementSpeed(float movementSpeed) { m_movementSpeed = movementSpeed; }

    float getSensitivity() const { return m_sensitivity; }
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }

    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }

    float getFov() const { return m_fov; }
    void setFov(float fov) { m_fov = fov; }

    glm::mat4 getProjection() const { return m_projection; }
    glm::mat4 getView() const { return m_view; }

    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getUp() const { return m_up; }

    void move(CameraDirection direction, float deltaTime);

    void update(float aspect, const glm::vec2& mouseDelta, const glm::vec2& scrollDelta);

private:
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{0.0f};
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    float m_sensitivity = 0.01f;
    float m_movementSpeed = 35.0f;

    glm::vec3 m_position{0.0f};

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;

    float m_fov = 80.0f;

    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};
};

class SphericalCamera {
public:
    float getSensitivity() const { return m_sensitivity; }
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }

    float getZoomSpeed() const { return m_zoomSpeed; }
    void setZoomSpeed(float zoomSpeed) { m_zoomSpeed = zoomSpeed; }

    float getMaxRadius() const { return m_maxRadius; }
    void setMaxRadius(float maxRadius) { m_maxRadius = maxRadius; }

    float getFov() const { return m_fov; }
    void setFov(float fov) { m_fov = fov; }

    glm::vec3 getTarget() const { return m_target; }
    void setTarget(const glm::vec3& target) { m_target = target; }

    glm::vec3 getPosition() const { return m_position; }

    glm::mat4 getProjection() const { return m_projection; }
    glm::mat4 getView() const { return m_view; }

    glm::vec3 getFront() const { return m_front; }

    float getPhi() const { return m_phi; }

    void update(float aspect, const glm::vec2& mouseDelta, const glm::vec2& scrollDelta);

private:
    glm::vec3 m_front{0.0f};

    float m_sensitivity = 0.01f;
    float m_zoomSpeed = 2.0f;
    float m_maxRadius = 40.0f;

    float m_radius = 5.0f;
    float m_theta = 0.0f;
    float m_phi = 0.0f;

    float m_fov = 80.0f;

    glm::vec3 m_position{0.0f};
    glm::vec3 m_target{0.0f};

    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};
};

} // namespace core::camera