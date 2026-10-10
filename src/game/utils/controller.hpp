#pragma once

#include "window/window.hpp"

#include "core/camera.hpp"

#include "../prefabs/rig.hpp"

#include <tuple>

namespace game::utils {

class Controller {
public:
    Controller(win::Window& window, const std::shared_ptr<prefabs::Rig>& localRig);
    ~Controller();

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    glm::mat4 getCameraProjection() const;
    glm::mat4 getCameraView() const;
    
    glm::vec3 getCameraPosition() const;
    std::tuple<glm::vec3, glm::vec3> getCameraLook() const;

    bool isFreeCameraMode() const { return m_freeCameraMode; }

    void update();

private:
    win::Window& m_window;

    std::unique_ptr<core::camera::SphericalCamera> m_sphericalCamera;
    std::unique_ptr<core::camera::FreeCamera> m_freeCamera;

    const std::shared_ptr<prefabs::Rig>& m_localRig;

    bool m_freeCameraMode = false;
};

} // namespace game::utils
