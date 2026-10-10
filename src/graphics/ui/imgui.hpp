#pragma once

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>

#include "../vk/device.hpp"
#include "../managers/bindless_manager.hpp"

#include <vector>

namespace gfx::ui {

class Imgui {
public:

    struct Vertex {
        glm::vec2 position{};
        glm::vec2 uv{};
        uint32_t color{};
        uint32_t texIndex{0};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 proj;
    };

    Imgui(win::Window& window, vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager);
    ~Imgui();

    Imgui(const Imgui&) = delete;
    Imgui& operator=(const Imgui&) = delete;

    void update();

    void draw(VkCommandBuffer commandBuffer);

private:
    win::Window& m_window;
    vk::Device& m_device;
    mngrs::BindlessManager& m_bindlessManager;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    std::unique_ptr<vk::Buffer> m_indexBuffer;

    uint32_t m_fontTextureIndex = 0;

    void initImGui();

    void createVertexBuffer();
    void createIndexBuffer();
    void createFontTexture(vk::Sampler& sampler);

    void uploadDrawData(ImDrawData* drawData);

    static const char* getClipboardText(void* userData);
    static void setClipboardText(void* userData, const char* text);
};

} // namespace gfx::ui