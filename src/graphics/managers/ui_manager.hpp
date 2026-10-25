#pragma once

#include "../ui/imgui.hpp"
#include "../ui/text.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace gfx::mngrs {

class UiManager {
public:
    using DesignCallback = std::function<void()>;

    enum class DockSlot {
        None,
        Left,
        Right,
        Top,
        Bottom
    };

    struct ImguiWindow {
        std::string title;
        DesignCallback design;
        ImGuiWindowFlags flags = 0;
        bool* open = nullptr;
        DockSlot dockSlot = DockSlot::None;
        float dockRatio = 0.5f;
    };

    UiManager(vk::Device& device, BindlessManager& bindlessManager);
    ~UiManager();

    UiManager(const UiManager&) = delete;
    UiManager& operator=(const UiManager&) = delete;

    uint32_t getTextureIndex(const std::string& name) const;

    void addImguiWindow(const ImguiWindow& window) {
        m_imguiWindows.push_back(window);
    }
    void setImguiMenuBar(const DesignCallback& design) {
        m_imguiMenuBar = design;
    }
    void setImguiOverlay(const DesignCallback& design) {
        m_imguiOverlay = design;
    }

    void loadText(
        const std::string& name, vk::Sampler& sampler,
        const std::string& fontPath
    );
    void loadImgui(win::Window& window, vk::Sampler& sampler);

    void update();

    void drawText(
        VkCommandBuffer commandBuffer,
        const std::vector<ui::Text::InstanceContent>& instancesData
    );
    void drawImgui(VkCommandBuffer commandBuffer);

private:
    vk::Device& m_device;
    BindlessManager& m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<ui::Text>> m_texts;
    std::unique_ptr<ui::Imgui> m_imgui;

    std::vector<ImguiWindow> m_imguiWindows;
    DesignCallback m_imguiMenuBar;
    DesignCallback m_imguiOverlay;

    bool m_dockLayoutBuilt = false;

    void setupDockspace();
};

} // namespace gfx::mngrs