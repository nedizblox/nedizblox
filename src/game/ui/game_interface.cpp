#include "game_interface.hpp"

#include <imgui/ImGuiNotify.hpp>

namespace game::ui {

GameInterface::GameInterface(
    gfx::mngrs::UiManager& uiManager, multiplayer::ClientReplicator& replicator
) : m_uiManager(uiManager), m_replicator(replicator) {
    m_replicator.getClient()->setNetworkMessageCallback(
        [](const std::string& message)
        {
            ImGui::InsertNotification(
                {ImGuiToastType::Info, 120000, message.c_str()}
            );
        }
    );
}

GameInterface::~GameInterface() {}

// TODO

} // namespace game::ui