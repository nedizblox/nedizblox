#pragma once

#include "window/window.hpp"

#include "graphics/ui/imgui.hpp"

#include "core/camera.hpp"

#include "../prefabs/rig.hpp"

namespace game::utils {

class Controller {
public:
    Controller(
        win::Window& window, gfx::ui::Imgui& imgui, core::camera::FreeCamera& freeCamera,
        core::camera::SphericalCamera& sphericalCamera, const std::shared_ptr<prefabs::Rig>& localRig);
    ~Controller();

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    bool isFreeCameraMode() const { return m_freeCameraMode; }

    void update();

private:
    win::Window& m_window;
    gfx::ui::Imgui& m_imgui;

    core::camera::FreeCamera& m_freeCamera;
    core::camera::SphericalCamera& m_sphericalCamera;

    const std::shared_ptr<prefabs::Rig>& m_localRig;

    bool m_freeCameraMode = false;
};

} // namespace game::utils