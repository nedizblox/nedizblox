#include "debug_tools.hpp"

namespace game::ui {

DebugTools::DebugTools(gfx::mngrs::UiManager& uiManager, mngrs::RenderManager& renderManager, multiplayer::Ownership& ownership, net::Client& client) :
    m_renderManager(renderManager), m_ownership(ownership), m_client(client) {
    createGUIs(uiManager);
}

DebugTools::~DebugTools() {}

void DebugTools::createGUIs(gfx::mngrs::UiManager& uiManager) {
    uiManager.addImguiWindow(
        {"Debug Tools",
         [this]() {
             if (ImGui::BeginTabBar("DEBUGTOOLS_BAR")) {
                 drawNetworkTab();
                 drawRenderTab();
                 drawOwnershipTab();
                 drawTexturesTab();
                 ImGui::EndTabBar();
             }
         },
         ImGuiWindowFlags_AlwaysAutoResize});
}

void DebugTools::drawNetworkTab() {
    if (ImGui::BeginTabItem("Network")) {
        ImGui::SeparatorText("Server");
        ImGui::Text("Address: %s", m_client.getServerAddress().c_str());
        ImGui::Text("Port: %hu", m_client.getServerPort());

        ImGui::SeparatorText("Client");
        ImGui::Text("Player nickname: %s", m_client.getPlayerNickname().c_str());
        ImGui::Text("Player id: %u", m_client.getPlayerId());

        ImGui::SeparatorText("Packets");
        ImGui::Text("Ping: %u ms", m_client.getPingMs());
        ImGui::Text("Sent: %.2f KB/s", m_client.getSentKBps());
        ImGui::Text("Recv: %.2f KB/s", m_client.getRecvKBps());

        ImGui::EndTabItem();
    }
}

void DebugTools::drawRenderTab() {
    if (ImGui::BeginTabItem("Render")) {
        

        ImGui::EndTabItem();
    }
}

void DebugTools::drawOwnershipTab() {
    if (ImGui::BeginTabItem("Ownership")) {
        ImGui::SeparatorText("Config");
        ImGui::Text("Max locally owned objects: %zu", m_ownership.kMaxLocallyOwnedObjects);
        ImGui::Text("Max collision depth: %d", m_ownership.kMaxCollisionDepth);
        ImGui::Text("Release distance sequence: %.2f", m_ownership.kReleaseDistanceSq);
        ImGui::Text("Network send interval: %.2f", m_ownership.kNetSendInterval);

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
        ImGui::InputScalar("Index", ImGuiDataType_U32, &m_currentTexIndex, &step, &stepFast, "%u");

        ImGui::EndTabItem();
    }
}

} // namespace game::ui