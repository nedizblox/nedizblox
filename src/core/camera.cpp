#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace core::camera {

void FreeCamera::move(CameraDirection direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (direction == FORWARD) {
        position += m_front * velocity;
    }
    if (direction == BACKWARD) {
        position -= m_front * velocity;
    }
    if (direction == LEFT) {
        position -= m_right * velocity;
    }
    if (direction == RIGHT) {
        position += m_right * velocity;
    }
}

void FreeCamera::update(float fov, float aspect, float near, float far, glm::vec2 mouseDelta) {
    m_yaw += mouseDelta.x * sensitivity;
    m_pitch += mouseDelta.y * sensitivity;
    m_pitch = glm::clamp(m_pitch, -glm::radians(89.0f), glm::radians(89.0f));

    glm::vec3 front;
    front.x = glm::cos(m_yaw) * glm::cos(m_pitch);
    front.y = glm::sin(m_pitch);
    front.z = glm::sin(m_yaw) * glm::cos(m_pitch);

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));

    m_projection = glm::perspective(fov, aspect, near, far);
    m_projection[1][1] *= -1; // inverse for vulkan

    m_view = glm::lookAt(position, position + m_front, m_up);
}

void SphericalCamera::update(float fov, float aspect, float near, float far, glm::vec2 mouseDelta, glm::vec2 scrollDelta) {
    m_radius -= scrollDelta.y * zoomSpeed * (m_radius * 0.1f);
    m_radius = glm::clamp(m_radius, 0.1f, maxRadius);

    m_phi -= mouseDelta.x * sensitivity;
    m_theta -= mouseDelta.y * sensitivity;
    m_theta = glm::clamp(m_theta, -glm::radians(89.0f), glm::radians(89.0f));

    glm::vec3 front;
    front.x = m_radius * glm::cos(m_theta) * glm::sin(m_phi);
    front.y = m_radius * glm::sin(m_theta);
    front.z = m_radius * glm::cos(m_theta) * glm::cos(m_phi);

    glm::vec3 eye = target + front;

    m_projection = glm::perspective(fov, aspect, near, far);
    m_projection[1][1] *= -1; // inverse for vulkan

    m_view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace core::camera