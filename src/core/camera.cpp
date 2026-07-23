#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace core::camera {

void FreeCamera::move(CameraDirection direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;
    if (direction == FORWARD) {
        m_position += m_front * velocity;
    }
    if (direction == BACKWARD) {
        m_position -= m_front * velocity;
    }
    if (direction == LEFT) {
        m_position -= m_right * velocity;
    }
    if (direction == RIGHT) {
        m_position += m_right * velocity;
    }
}

void FreeCamera::update(float aspect, glm::vec2 mouseDelta) {
    m_yaw += mouseDelta.x * m_sensitivity;
    m_pitch += mouseDelta.y * m_sensitivity;
    m_pitch = glm::clamp(m_pitch, -glm::radians(89.0f), glm::radians(89.0f));

    glm::vec3 front;
    front.x = glm::cos(m_yaw) * glm::cos(m_pitch);
    front.y = glm::sin(m_pitch);
    front.z = glm::sin(m_yaw) * glm::cos(m_pitch);

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));

    m_projection = glm::perspective(glm::radians(m_fov), aspect, 0.1f, 1000.0f);
    m_projection[1][1] *= -1; // inverse for vulkan

    m_view = glm::lookAt(m_position, m_position + m_front, m_up);
}

void SphericalCamera::update(float aspect, glm::vec2 mouseDelta, glm::vec2 scrollDelta) {
    m_radius -= scrollDelta.y * m_zoomSpeed * (m_radius * 0.1f);
    m_radius = glm::clamp(m_radius, 0.1f, m_maxRadius);

    m_phi -= mouseDelta.x * m_sensitivity;
    m_theta -= mouseDelta.y * m_sensitivity;
    m_theta = glm::clamp(m_theta, -glm::radians(89.0f), glm::radians(89.0f));

    glm::vec3 front;
    front.x = m_radius * glm::cos(m_theta) * glm::sin(m_phi);
    front.y = m_radius * glm::sin(m_theta);
    front.z = m_radius * glm::cos(m_theta) * glm::cos(m_phi);

    m_position = m_target + front;

    m_projection = glm::perspective(glm::radians(m_fov), aspect, 0.1f, 1000.0f);
    m_projection[1][1] *= -1; // inverse for vulkan

    m_view = glm::lookAt(m_position, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace core::camera