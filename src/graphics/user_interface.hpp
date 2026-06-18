#pragma once

#include "managers/bindless_manager.hpp"
#include "vk/device.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#include <nuklear/nuklear.h>

#include <functional>

namespace gfx {

class UserInterface {
public:
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
        glm::u8vec4 color;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 proj{1.0f};
        alignas(4) uint32_t texIndex{0};
    };

    struct UI {
        std::string title;
        struct nk_rect bounds;
        std::function<void(struct nk_context* ctx)> build;
        nk_flags flags;
    };

    UserInterface(vk::Device& device, win::Window& window, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager);
    ~UserInterface();

    bool hasFocus() const { return m_ctx.text_edit.active != 0; }

    uint32_t getFontTextureIndex() const { return m_fontTextureIndex; }

    void addUI(const UI& ui) { m_uis.push_back(ui); }

    void updateInput();

    void draw(VkCommandBuffer commandBuffer);

private:
    vk::Device& m_device;
    win::Window& m_window;
    vk::Sampler& m_sampler;
    mngrs::BindlessManager& m_bindlessManager;

    struct nk_context m_ctx;
    struct nk_font_atlas m_atlas;
    struct nk_buffer m_cmds;

    struct nk_draw_null_texture m_nullTexture;

    std::vector<UI> m_uis;

    uint32_t m_fontTextureIndex;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    std::unique_ptr<vk::Buffer> m_indexBuffer;

    void convertToBuffers();

    void init();
    void createVertexBuffer();
    void createIndexBuffer();
};

} // namespace gfx::ui