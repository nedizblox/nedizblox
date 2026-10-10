#include "controller.hpp"

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>

namespace game::utils {

Controller::Controller(win::Window& window, const std::shared_ptr<prefabs::Rig>& localRig) :
    m_window(window),
    m_localRig(localRig) {
    m_freeCamera = std::make_unique<core::camera::FreeCamera>();
    m_sphericalCamera = std::make_unique<core::camera::SphericalCamera>();
}

Controller::~Controller() {}

glm::mat4 Controller::getCameraProjection() const {
    if (m_freeCameraMode) {
        return m_freeCamera->getProjection();
    } else {
        return m_sphericalCamera->getProjection();
    }
}

glm::mat4 Controller::getCameraView() const {
    if (m_freeCameraMode) {
        return m_freeCamera->getView();
    } else {
        return m_sphericalCamera->getView();
    }
}

glm::vec3 Controller::getCameraPosition() const {
    if (m_freeCameraMode) {
        return m_freeCamera->getPosition();
    } else {
        return m_sphericalCamera->getPosition();
    }
}

std::tuple<glm::vec3, glm::vec3> Controller::getCameraLook() const {
    if (m_freeCameraMode) {
        return {m_freeCamera->getFront(), m_freeCamera->getUp()};
    } else {
        return {m_sphericalCamera->getFront(), glm::vec3(0.0f, 1.0f, 0.0f)};
    }
}

void Controller::update() {
    ImGuiIO& io = ImGui::GetIO();
    
    bool isGizmoActive = ImGuizmo::IsUsing();

    float phi = m_sphericalCamera->getPhi();

    if (!io.WantCaptureKeyboard && !isGizmoActive) {
        if (m_window.isKeyJustPressed(SDL_SCANCODE_F3))
            m_freeCameraMode = !m_freeCameraMode;

        if (!m_freeCameraMode) {
            if (m_window.isKeyPressed(SDL_SCANCODE_W))
                m_localRig->move(enums::RigMoveDirection::Forward, phi);
            if (m_window.isKeyPressed(SDL_SCANCODE_S))
                m_localRig->move(enums::RigMoveDirection::Backward, phi);
            if (m_window.isKeyPressed(SDL_SCANCODE_A))
                m_localRig->move(enums::RigMoveDirection::Left, phi);
            if (m_window.isKeyPressed(SDL_SCANCODE_D))
                m_localRig->move(enums::RigMoveDirection::Right, phi);
            if (m_window.isKeyPressed(SDL_SCANCODE_SPACE))
                m_localRig->jump();
        } else {
            float dt = m_window.getDeltaTime();

            if (m_window.isKeyPressed(SDL_SCANCODE_W))
                m_freeCamera->move(core::camera::FreeCamera::CameraDirection::Forward, dt);
            if (m_window.isKeyPressed(SDL_SCANCODE_S))
                m_freeCamera->move(core::camera::FreeCamera::CameraDirection::Backward, dt);
            if (m_window.isKeyPressed(SDL_SCANCODE_A))
                m_freeCamera->move(core::camera::FreeCamera::CameraDirection::Left, dt);
            if (m_window.isKeyPressed(SDL_SCANCODE_D))
                m_freeCamera->move(core::camera::FreeCamera::CameraDirection::Right, dt);
        }
    }

    glm::vec2 mouseDelta{};
    glm::vec2 scrollDelta{};
    if (!io.WantCaptureMouse && !isGizmoActive) {
        mouseDelta = m_window.isMouseButtonPressed(SDL_BUTTON_RIGHT) ? m_window.getMouseDelta()
                                                                            : glm::vec2(0.0f);
        scrollDelta = m_window.getScrollDelta();
    }

    if (m_localRig)
        m_sphericalCamera->setTarget(m_localRig->getPivotPosition());

    if (m_freeCameraMode)
        m_freeCamera->update(m_window.getAspect(), mouseDelta, scrollDelta);
    else
        m_sphericalCamera->update(m_window.getAspect(), mouseDelta, scrollDelta);
}

} // namespace game::utils
