#pragma once

#include "../managers/instance_manager.hpp"

#include "network/client.hpp"

#include "graphics/ui/imgui.hpp"

namespace game::utils {

class DebugTools {
public:
    DebugTools(gfx::ui::Imgui& imgui, mngrs::InstanceManager& instanceManager, net::Client& client);
    ~DebugTools();

    DebugTools(const DebugTools&) = delete;
    DebugTools& operator=(const DebugTools&) = delete;

private:
    gfx::ui::Imgui& m_imgui;
    mngrs::InstanceManager& m_instanceManager;
    net::Client& m_client;

    void createGUIs();

    void drawNetworkTab();

    void drawInstancesTab();
};

} // namespace game::utils