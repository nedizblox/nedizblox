#pragma once

#include <imgui/imgui.h>

#include "../vk/device.hpp"
#include "../managers/bindless_manager.hpp"

#include <vector>
#include <string>
#include <functional>

namespace gfx::ui {

class Imgui {
public:
    using DesignCallback = std::function<void()>;

    struct PushConstantObject {
        alignas(16) glm::mat4 proj;
        alignas(4) uint32_t texIndex;
    };

    struct GUI {
        std::string title;
        DesignCallback design;
    };

    Imgui(win::Window& window, vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager);
    ~Imgui();

    Imgui(const Imgui&) = delete;
    Imgui& operator=(const Imgui&) = delete;

    uint32_t getTextureIndex() const { return m_fontTextureIndex; }

    bool isKeyboardFocused() const { return m_isKeyboardFocused; }
    bool isMouseFocused() const { return m_isMouseFocused; }

    void showDemoWindow(bool show) { m_showDemoWindow = show; }

    void addGUI(const GUI& gui) { m_guis.push_back(gui); }

    void update();

    void draw(VkCommandBuffer commandBuffer);

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

private:
    win::Window& m_window;
    vk::Device& m_device;
    mngrs::BindlessManager& m_bindlessManager;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    std::unique_ptr<vk::Buffer> m_indexBuffer;

    uint32_t m_fontTextureIndex = 0;

    std::vector<GUI> m_guis;

    bool m_isKeyboardFocused = false;
    bool m_isMouseFocused = false;

    bool m_showDemoWindow = false;

    void initImGui();

    void createVertexBuffer();
    void createIndexBuffer();
    void createFontTexture(vk::Sampler& sampler);

    void uploadDrawData(ImDrawData* drawData);

    static const char* getClipboardText(void* userData);
    static void setClipboardText(void* userData, const char* text);
};

} // namespace gfx::ui