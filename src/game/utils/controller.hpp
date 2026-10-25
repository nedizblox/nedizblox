#pragma once

#include "window/window.hpp"

#include "core/camera.hpp"

#include "../managers/humanoid_manager.hpp"
#include "../types/rig.hpp"

#include <tuple>

namespace game::utils {

class Controller {
public:
    Controller(
        win::Window& window, mngrs::HumanoidManager& humanoids,
        const std::shared_ptr<types::Rig>& localRig
    );
    ~Controller();

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    glm::mat4 getCameraProjection() const;
    glm::mat4 getCameraView() const;

    glm::vec3 getCameraPosition() const;
    std::tuple<glm::vec3, glm::vec3> getCameraLook() const;

    bool isFreeCameraMode() const {
        return m_freeCameraMode;
    }
    void setFreeCameraMode(bool mode) {
        m_freeCameraMode = mode;
    }

    bool isKeyDown(SDL_Scancode key) const {
        return m_window.isKeyPressed(key);
    }

    void setLocalRig(const std::shared_ptr<types::Rig>& rig) {
        m_localRig = rig;
    }

    void update();

private:
    win::Window& m_window;

    std::unique_ptr<core::camera::SphericalCamera> m_sphericalCamera;
    std::unique_ptr<core::camera::FreeCamera> m_freeCamera;

    mngrs::HumanoidManager& m_humanoids;
    std::shared_ptr<types::Rig> m_localRig;

    bool m_freeCameraMode = false;

    glm::vec2 getMoveAxis() const;
};

} // namespace game::utils