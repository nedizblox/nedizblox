#pragma once

#include "../managers/render_manager.hpp"

#include "../multiplayer/multiplayer.hpp"

#include "graphics/managers/ui_manager.hpp"

namespace game::ui {

class DebugTools {
public:
    DebugTools(
        gfx::mngrs::UiManager& uiManager, mngrs::RenderManager& renderManager,
        multiplayer::ClientOwnership& ownership, multiplayer::ClientReplicator& replicator
    );
    ~DebugTools();

    DebugTools(const DebugTools&) = delete;
    DebugTools& operator=(const DebugTools&) = delete;

private:
    mngrs::RenderManager& m_renderManager;
    multiplayer::ClientOwnership& m_ownership;
    multiplayer::ClientReplicator& m_replicator;

    void createGUIs(gfx::mngrs::UiManager& uiManager);

    void drawNetworkTab();

    void drawRenderTab();

    void drawOwnershipTab();

    void drawTexturesTab();
    uint32_t m_currentTexIndex = 0;
};

} // namespace game::ui