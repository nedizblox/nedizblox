#include "debug_tools.hpp"

namespace game::ui {

DebugTools::DebugTools(
    gfx::mngrs::UiManager& uiManager, mngrs::RenderManager& renderManager,
    multiplayer::ClientOwnership& ownership, multiplayer::ClientReplicator& replicator
) :
    m_renderManager(renderManager), m_ownership(ownership),
    m_replicator(replicator) {
    createGUIs(uiManager);
}

DebugTools::~DebugTools() {
}

void DebugTools::createGUIs(gfx::mngrs::UiManager& uiManager) {
    uiManager.addImguiWindow(
        {"Debug Tools",
         [this]()
         {
             if (ImGui::BeginTabBar("DEBUGTOOLS_BAR")) {
                 drawNetworkTab();
                 drawRenderTab();
                 drawOwnershipTab();
                 drawTexturesTab();
                 ImGui::EndTabBar();
             }
         },
         ImGuiWindowFlags_AlwaysAutoResize}
    );
}

void DebugTools::drawNetworkTab() {
    if (ImGui::BeginTabItem("Network")) {
        if (!m_replicator.isOfflineMode()) {
            net::Client* client = m_replicator.getClient();

            ImGui::SeparatorText("Server");
            ImGui::Text("Address: %s", client->getServerAddress().c_str());
            ImGui::Text("Port: %hu", client->getServerPort());

            ImGui::SeparatorText("Client");
            ImGui::Text(
                "Player nickname: %s", client->getPlayerNickname().c_str()
            );
            ImGui::Text("Player id: %u", client->getPlayerId());

            ImGui::SeparatorText("Packets");
            ImGui::Text("Ping: %u ms", client->getPingMs());
            ImGui::Text("Sent: %.2f KB/s", client->getSentKBps());
            ImGui::Text("Recv: %.2f KB/s", client->getRecvKBps());
        } else {
            ImGui::SeparatorText("Server");
            ImGui::Text("Address: N/A");
            ImGui::Text("Port: N/A");

            ImGui::SeparatorText("Client");
            ImGui::Text("Player nickname: Player0");
            ImGui::Text("Player id: 0");

            ImGui::SeparatorText("Packets");
            ImGui::Text("Ping: 0 ms");
            ImGui::Text("Sent: 0.00 KB/s");
            ImGui::Text("Recv: 0.00 KB/s");
        }

        ImGui::EndTabItem();
    }
}

void DebugTools::drawRenderTab() {
    if (ImGui::BeginTabItem("Render")) {
        ImGui::SeparatorText("Rendering tree");

        ImGui::Text(
            "Map rebuild count: %zu", m_renderManager.getMapRebuildCount()
        );
        ImGui::Text(
            "Gui rebuild count: %zu", m_renderManager.getGuiRebuildCount()
        );

        ImGui::SeparatorText("Culling");

        ImGui::Text("Max instances per bucket: %zu", m_renderManager.getMaxInstancesPerBucket());
        ImGui::Text("Total instance count: %zu", m_renderManager.getTotalInstanceCount());
        ImGui::Text("Visible instance count: %zu", m_renderManager.getVisibleInstanceCount());

        ImGui::EndTabItem();
    }
}

void DebugTools::drawOwnershipTab() {
    if (ImGui::BeginTabItem("Ownership")) {
        ImGui::SeparatorText("Config");
        ImGui::Text(
            "Max locally owned objects: %zu",
            m_ownership.kMaxLocallyOwnedObjects
        );
        ImGui::Text("Max collision depth: %d", m_ownership.kMaxCollisionDepth);
        ImGui::Text(
            "Release distance sequence: %.2f", m_ownership.kReleaseDistanceSq
        );
        ImGui::Text(
            "Network send interval: %.2f", m_ownership.kNetSendInterval
        );

        auto ownedObjects = m_ownership.getOwnedObjectIds();

        ImGui::SeparatorText("Current state");
        ImGui::Text("Number of owned objects: %zu", ownedObjects.size());

        ImGui::EndTabItem();
    }
}

void DebugTools::drawTexturesTab() {
    if (ImGui::BeginTabItem("Textures")) {
        ImGui::Image(m_currentTexIndex, ImVec2(100, 100));

        uint32_t step = 1;
        uint32_t stepFast = 10;
        ImGui::InputScalar(
            "Index", ImGuiDataType_U32, &m_currentTexIndex, &step, &stepFast,
            "%u"
        );

        ImGui::EndTabItem();
    }
}

} // namespace game::ui