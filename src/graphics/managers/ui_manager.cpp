#include "ui_manager.hpp"

#include <imgui/imgui_internal.h>

#include <imgui/ImGuiNotify.hpp>

namespace gfx::mngrs {

namespace {

ImGuiDir slotToSplitDir(UiManager::DockSlot slot) {
    switch (slot) {
        case UiManager::DockSlot::Left:
            return ImGuiDir_Left;
        case UiManager::DockSlot::Right:
            return ImGuiDir_Right;
        case UiManager::DockSlot::Top:
            return ImGuiDir_Up;
        case UiManager::DockSlot::Bottom:
            return ImGuiDir_Down;
        default:
            return ImGuiDir_Right;
    }
}

ImGuiDir slotToStackDir(UiManager::DockSlot slot) {
    switch (slot) {
        case UiManager::DockSlot::Left:
        case UiManager::DockSlot::Right:
            return ImGuiDir_Up;
        default:
            return ImGuiDir_Left;
    }
}

} // namespace

UiManager::UiManager(vk::Device& device, BindlessManager& bindlessManager) :
    m_device(device), m_bindlessManager(bindlessManager) {
}

UiManager::~UiManager() {
}

uint32_t UiManager::getTextureIndex(const std::string& name) const {
    if (auto it = m_texts.find(name); it != m_texts.end()) {
        return it->second->getFontTextureIndex();
    }

    return 0;
}

void UiManager::loadText(
    const std::string& name, vk::Sampler& sampler, const std::string& fontPath
) {
    if (m_texts.contains(name))
        return;

    m_texts[name] = std::make_unique<ui::Text>(
        m_device, sampler, m_bindlessManager, fontPath
    );
}

void UiManager::loadImgui(win::Window& window, vk::Sampler& sampler) {
    m_imgui = std::make_unique<ui::Imgui>(
        window, m_device, sampler, m_bindlessManager
    );
}

void UiManager::update() {
    m_imgui->update();
}

void UiManager::drawText(
    VkCommandBuffer commandBuffer,
    const std::vector<ui::Text::InstanceContent>& instancesData
) {
    for (auto& [name, text] : m_texts) {
        text->draw(commandBuffer, instancesData);
    }
}

void UiManager::setupDockspace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##DockspaceHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("MainDockspace");

    if (!m_dockLayoutBuilt) {
        m_dockLayoutBuilt = true;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(
            dockspaceId, ImGuiDockNodeFlags_DockSpace |
                             ImGuiDockNodeFlags_PassthruCentralNode
        );
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        ImGuiID centralId = dockspaceId;

        constexpr float kColumnWidthRatio = 0.22f;
        constexpr float kRowHeightRatio = 0.28f;

        for (DockSlot slot :
             {DockSlot::Left, DockSlot::Right, DockSlot::Top,
              DockSlot::Bottom}) {
            std::vector<const ImguiWindow*> windowsInSlot;
            for (const auto& gui : m_imguiWindows) {
                if (gui.dockSlot == slot) {
                    windowsInSlot.push_back(&gui);
                }
            }

            if (windowsInSlot.empty())
                continue;

            bool isColumn = (slot == DockSlot::Left || slot == DockSlot::Right);
            float slotRatio = isColumn ? kColumnWidthRatio : kRowHeightRatio;

            ImGuiID slotId = ImGui::DockBuilderSplitNode(
                centralId, slotToSplitDir(slot), slotRatio, nullptr, &centralId
            );

            ImGuiID current = slotId;
            for (size_t i = 0; i < windowsInSlot.size(); i++) {
                if (i + 1 < windowsInSlot.size()) {
                    ImGuiID rest;
                    ImGuiID first = ImGui::DockBuilderSplitNode(
                        current, slotToStackDir(slot),
                        windowsInSlot[i]->dockRatio, nullptr, &rest
                    );
                    ImGui::DockBuilderDockWindow(
                        windowsInSlot[i]->title.c_str(), first
                    );
                    current = rest;
                } else {
                    ImGui::DockBuilderDockWindow(
                        windowsInSlot[i]->title.c_str(), current
                    );
                }
            }
        }

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(
        dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode
    );

    ImGui::End();
}

void UiManager::drawImgui(VkCommandBuffer commandBuffer) {
    setupDockspace();

    for (const auto& gui : m_imguiWindows) {
        if (gui.open && !*gui.open)
            continue;

        if (ImGui::Begin(gui.title.c_str(), gui.open, gui.flags))
            gui.design();
        ImGui::End();
    }

    if (m_imguiMenuBar && ImGui::BeginMainMenuBar()) {
        m_imguiMenuBar();
        ImGui::EndMainMenuBar();
    }

    if (m_imguiOverlay)
        m_imguiOverlay();

    ImGui::RenderNotifications();

    m_imgui->draw(commandBuffer);
}

} // namespace gfx::mngrs