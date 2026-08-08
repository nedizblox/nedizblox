#include "controller.hpp"

namespace game::utils {

Controller::Controller(
    win::Window& window, gfx::ui::Imgui& imgui, core::camera::FreeCamera& freeCamera,
    core::camera::SphericalCamera& sphericalCamera, const std::shared_ptr<prefabs::Rig>& localRig) :
    m_window(window),
    m_imgui(imgui),
    m_freeCamera(freeCamera),
    m_sphericalCamera(sphericalCamera),
    m_localRig(localRig) {}

Controller::~Controller() {}

void Controller::update() {
    float phi = m_sphericalCamera.getPhi();

    if (!m_imgui.isKeyboardFocused()) {
        if (m_window.isKeyJustPressed(GLFW_KEY_F3))
            m_freeCameraMode = !m_freeCameraMode;

        if (!m_freeCameraMode) {
            if (m_window.isKeyPressed(GLFW_KEY_W))
                m_localRig->move(enums::RigMoveDirection::Forward, phi);
            if (m_window.isKeyPressed(GLFW_KEY_S))
                m_localRig->move(enums::RigMoveDirection::Backward, phi);
            if (m_window.isKeyPressed(GLFW_KEY_A))
                m_localRig->move(enums::RigMoveDirection::Left, phi);
            if (m_window.isKeyPressed(GLFW_KEY_D))
                m_localRig->move(enums::RigMoveDirection::Right, phi);
            if (m_window.isKeyPressed(GLFW_KEY_SPACE))
                m_localRig->jump();
        } else {
            float dt = m_window.getDeltaTime();

            if (m_window.isKeyPressed(GLFW_KEY_W))
                m_freeCamera.move(core::camera::FreeCamera::CameraDirection::Forward, dt);
            if (m_window.isKeyPressed(GLFW_KEY_S))
                m_freeCamera.move(core::camera::FreeCamera::CameraDirection::Backward, dt);
            if (m_window.isKeyPressed(GLFW_KEY_A))
                m_freeCamera.move(core::camera::FreeCamera::CameraDirection::Left, dt);
            if (m_window.isKeyPressed(GLFW_KEY_D))
                m_freeCamera.move(core::camera::FreeCamera::CameraDirection::Right, dt);
        }
    }

    glm::vec2 mouseDelta{};
    glm::vec2 scrollDelta{};
    if (!m_imgui.isMouseFocused()) {
        mouseDelta = m_window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT) ? m_window.getMouseDelta()
                                                                            : glm::vec2(0.0f);
        scrollDelta = m_window.getScrollDelta();
    }

    if (m_freeCameraMode)
        m_freeCamera.update(m_window.getAspect(), mouseDelta, scrollDelta);
    else
        m_sphericalCamera.update(m_window.getAspect(), mouseDelta, scrollDelta);
}

} // namespace game::utils