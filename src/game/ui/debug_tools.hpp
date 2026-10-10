#pragma once

#include "../managers/render_manager.hpp"

#include "../multiplayer/ownership.hpp"

#include "network/client.hpp"

#include "graphics/managers/ui_manager.hpp"

namespace game::ui {

class DebugTools {
public:
    DebugTools(gfx::mngrs::UiManager& uiManager, mngrs::RenderManager& renderManager, multiplayer::Ownership& ownership, net::Client& client);
    ~DebugTools();

    DebugTools(const DebugTools&) = delete;
    DebugTools& operator=(const DebugTools&) = delete;

private:
    mngrs::RenderManager& m_renderManager;
    multiplayer::Ownership& m_ownership;
    net::Client& m_client;

    void createGUIs(gfx::mngrs::UiManager& uiManager);

    void drawNetworkTab();

    void drawRenderTab();

    void drawOwnershipTab();

    void drawTexturesTab();
    uint32_t m_currentTexIndex = 0;
};

} // namespace game::ui