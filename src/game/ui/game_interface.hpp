#pragma once

#include "graphics/managers/ui_manager.hpp"

#include "../multiplayer/client_replicator.hpp"

#include <imgui/imgui.h>

namespace game::ui {

class GameInterface {
public:
    GameInterface(gfx::mngrs::UiManager& uiManager, multiplayer::ClientReplicator& replicator);
    ~GameInterface();

    GameInterface(const GameInterface&) = delete;
    GameInterface& operator=(const GameInterface&) = delete;

private:
    gfx::mngrs::UiManager& m_uiManager;
    multiplayer::ClientReplicator& m_replicator;
};

} // namespace game::ui