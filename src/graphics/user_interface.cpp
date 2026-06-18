#include "user_interface.hpp"

namespace gfx {

UserInterface::UserInterface(vk::Device& device, win::Window& window, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager) :
    m_device(device), m_window(window), m_sampler(sampler), m_bindlessManager(bindlessManager) {
    init();
    createVertexBuffer();
    createIndexBuffer();
}

UserInterface::~UserInterface() {
    nk_buffer_free(&m_cmds);
    nk_font_atlas_clear(&m_atlas);
    nk_free(&m_ctx);
}

void UserInterface::init() {
    nk_init_default(&m_ctx, nullptr);

    nk_style_default(&m_ctx);

    nk_font_atlas_init_default(&m_atlas);
    nk_font_atlas_begin(&m_atlas);
    struct nk_font* font = nk_font_atlas_add_from_file(&m_atlas, "assets/fonts/RobotoMono.ttf", 15.0f, nullptr);

    int w, h;
    const void* imageData = nk_font_atlas_bake(&m_atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(w * h * 4);

    vk::Buffer imageBuffer(
        m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    imageBuffer.uploadData(imageData, imageSize);

    auto fontTexture = std::make_unique<Texture>(
        m_device, m_sampler, imageBuffer, static_cast<uint32_t>(w), static_cast<uint32_t>(h),
        VK_FORMAT_R8G8B8A8_UNORM);
    m_fontTextureIndex = m_bindlessManager.addTexture(std::move(fontTexture));

    nk_font_atlas_end(&m_atlas, nk_handle_id(static_cast<int>(m_fontTextureIndex)), &m_nullTexture);

    nk_style_set_font(&m_ctx, &font->handle);

    nk_buffer_init_default(&m_cmds);
}

void UserInterface::createVertexBuffer() {
    VkDeviceSize bufferSize = 64 * 1024;

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

void UserInterface::createIndexBuffer() {
    VkDeviceSize bufferSize = 32 * 1024;

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

void UserInterface::convertToBuffers() {
    struct nk_convert_config config{};
    static const struct nk_draw_vertex_layout_element vertexLayout[]
        = {{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(Vertex, position)},
           {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(Vertex, uv)},
           {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(Vertex, color)},
           {NK_VERTEX_LAYOUT_END}};
    config.vertex_layout = vertexLayout;
    config.vertex_size = sizeof(Vertex);
    config.vertex_alignment = alignof(Vertex);
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.tex_null = m_nullTexture;

    struct nk_buffer vbuf, ibuf;
    nk_buffer_init_default(&vbuf);
    nk_buffer_init_default(&ibuf);

    nk_convert(&m_ctx, &m_cmds, &vbuf, &ibuf, &config);

    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(nk_buffer_total(&vbuf));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(nk_buffer_total(&ibuf));

    if (vertexSize > 0) {
        if (m_vertexBuffer->getSize() < vertexSize) {
            m_device.waitIdle();
            m_vertexBuffer = std::make_unique<vk::Buffer>(
                m_device, static_cast<VkDeviceSize>(vertexSize * 1.5f),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        }
        m_vertexBuffer->uploadData(nk_buffer_memory(&vbuf), vertexSize);
    }

    if (indexSize > 0) {
        if (m_indexBuffer->getSize() < indexSize) {
            m_device.waitIdle();
            m_indexBuffer = std::make_unique<vk::Buffer>(
                m_device, static_cast<VkDeviceSize>(indexSize * 1.5f),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        }
        m_indexBuffer->uploadData(nk_buffer_memory(&ibuf), indexSize);
    }

    nk_buffer_free(&vbuf);
    nk_buffer_free(&ibuf);
}

void UserInterface::updateInput() {
    nk_input_begin(&m_ctx);

    glm::i32vec2 mousePos = static_cast<glm::i32vec2>(m_window.getMousePosFb());
    nk_input_motion(&m_ctx, mousePos.x, mousePos.y);
    nk_input_button(&m_ctx, NK_BUTTON_LEFT, mousePos.x, mousePos.y, m_window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT));
    nk_input_button(&m_ctx, NK_BUTTON_RIGHT, mousePos.x, mousePos.y, m_window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT));

    for (auto codepoint : m_window.getInputCodepoints()) {
        nk_input_unicode(&m_ctx, codepoint);
    }

    nk_input_key(&m_ctx, NK_KEY_BACKSPACE, m_window.isKeyPressed(GLFW_KEY_BACKSPACE));
    nk_input_key(&m_ctx, NK_KEY_DEL, m_window.isKeyPressed(GLFW_KEY_DELETE));
    nk_input_key(&m_ctx, NK_KEY_ENTER, m_window.isKeyPressed(GLFW_KEY_ENTER));

    nk_input_key(&m_ctx, NK_KEY_LEFT, m_window.isKeyPressed(GLFW_KEY_LEFT));
    nk_input_key(&m_ctx, NK_KEY_RIGHT, m_window.isKeyPressed(GLFW_KEY_RIGHT));

    bool ctrl = m_window.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_window.isKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    nk_input_key(&m_ctx, NK_KEY_COPY, ctrl && m_window.isKeyPressed(GLFW_KEY_C));
    nk_input_key(&m_ctx, NK_KEY_PASTE, ctrl && m_window.isKeyPressed(GLFW_KEY_V));

    nk_input_end(&m_ctx);
}

void UserInterface::draw(VkCommandBuffer commandBuffer) {
    for (auto& ui : m_uis) {
        nk_begin(&m_ctx, ui.title.c_str(), ui.bounds, ui.flags);
        ui.build(&m_ctx);
        nk_end(&m_ctx);
    }

    convertToBuffers();

    if (!m_vertexBuffer || !m_indexBuffer) {
        nk_clear(&m_ctx);
        return;
    }

    VkBuffer vertexBuffer = m_vertexBuffer->getBuffer();
    VkDeviceSize offset = 0;

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), offset, VK_INDEX_TYPE_UINT16);

    const struct nk_draw_command* cmd;
    uint32_t indexOffset = 0;

    nk_draw_foreach(cmd, &m_ctx, &m_cmds) {
        if (!cmd->elem_count)
            continue;

        VkRect2D scissor{};
        scissor.offset.x = static_cast<int32_t>(glm::max(cmd->clip_rect.x, 0.0f));
        scissor.offset.y = static_cast<int32_t>(glm::max(cmd->clip_rect.y, 0.0f));
        scissor.extent.width = static_cast<uint32_t>(cmd->clip_rect.w);
        scissor.extent.height = static_cast<uint32_t>(cmd->clip_rect.h);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDrawIndexed(commandBuffer, cmd->elem_count, 1, indexOffset, 0, 0);

        indexOffset += cmd->elem_count;
    }

    nk_clear(&m_ctx);
}

std::vector<VkVertexInputBindingDescription> UserInterface::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> UserInterface::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)});

    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

    attributeDescriptions.push_back({2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex, color)});

    return attributeDescriptions;
}

} // namespace gfx::ui