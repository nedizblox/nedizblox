#include "debug_tools.hpp"

namespace game::utils {

DebugTools::DebugTools(gfx::ui::Imgui& imgui, mngrs::InstanceManager& instanceManager, net::Client& client) :
    m_imgui(imgui), m_instanceManager(instanceManager), m_client(client) {
    createGUIs();
}

DebugTools::~DebugTools() {}

void DebugTools::createGUIs() {
    m_imgui.addGUI({"Debug Tools", [this]() {
                        if (ImGui::BeginTabBar("DEBUGTOOLS_BAR")) {
                            drawNetworkTab();
                            drawInstancesTab();
                            ImGui::EndTabBar();
                        }
                    }});
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

void DebugTools::drawInstancesTab() {
    if (ImGui::BeginTabItem("Instances")) {
        ImGui::SeparatorText("Instance Manager");
        ImGui::Text("Map tree rebuilds: %zu", m_instanceManager.getMapRebuildCount());
        ImGui::Text("Gui tree rebuilds: %zu", m_instanceManager.getGuiRebuildCount());

        ImGui::EndTabItem();
    }
}

} // namespace game::utils