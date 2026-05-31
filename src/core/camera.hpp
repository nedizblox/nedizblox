#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace core::camera {

class FreeCamera {
public:
    enum CameraDirection { FORWARD, BACKWARD, LEFT, RIGHT };

    float sensitivity = 0.01f;
    float movementSpeed = 35.0f;

    glm::vec3 position{0.0f};

    glm::mat4 getProjection() const { return m_projection; }
    glm::mat4 getView() const { return m_view; }

    void move(CameraDirection direction, float deltaTime);

    void update(float fov, float aspect, float near, float far, glm::vec2 mouseDelta);

private:
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{0.0f};
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;

    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};
};

class SphericalCamera {
public:
    float sensitivity = 0.01f;
    float zoomSpeed = 2.0f;
    float maxRadius = 40.0f;

    glm::vec3 target{0.0f};

    glm::mat4 getProjection() const { return m_projection; }
    glm::mat4 getView() const { return m_view; }

    float getPhi() const { return m_phi; }

    void update(float fov, float aspect, float near, float far, glm::vec2 mouseDelta, glm::vec2 scrollDelta);

private:
    float m_radius = 5.0f;
    float m_theta = 0.0f;
    float m_phi = 0.0f;

    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};
};

} // namespace core::camera