#include "text.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstring>
#include <stdexcept>

namespace gfx::billb {

Text::Text(
    vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager,
    const std::string& fontPath, uint32_t maxChars) :
    m_device(device) {
    createVertexBuffer();
    createIndexBuffer(maxChars);
    createInstanceBuffer();
    loadFont(fontPath, sampler, bindlessManager);
}

Text::~Text() {} // buffers will be automatically destroyed

void Text::createVertexBuffer() {
    std::vector<glm::vec2> vertices = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(vertices.data(), bufferSize);

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
}

void Text::createIndexBuffer(uint32_t maxChars) {
    std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
    uint32_t indexCount = static_cast<uint32_t>(indices.size());

    for (uint32_t i = 0; i < maxChars; i++) {
        uint32_t offset = i * 4;

        indices.push_back(offset + 0);
        indices.push_back(offset + 1);
        indices.push_back(offset + 2);

        indices.push_back(offset + 2);
        indices.push_back(offset + 3);
        indices.push_back(offset + 0);
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(indices.data(), bufferSize);

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
}

void Text::createInstanceBuffer() {
    VkDeviceSize bufferSize = sizeof(glm::mat4) * 10000;

    m_instanceBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    m_instanceData = m_instanceBuffer->map();
}

void Text::loadFont(const std::string& fontPath, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("FreeType: Failed to init library");
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        FT_Done_FreeType(ft);
        throw std::runtime_error("FreeType: Failed to load font");
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    const uint32_t atlasWidth = 1024;
    const uint32_t atlasHeight = 1024;
    VkDeviceSize bufferSize = atlasWidth * atlasHeight;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    uint8_t* pData = static_cast<uint8_t*>(stagingBuffer.map());
    memset(pData, 0, bufferSize);

    uint32_t currentX = 0;
    uint32_t currentY = 0;
    uint32_t rowHeight = 0;

    for (uint8_t c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }

        FT_Bitmap& bitmap = face->glyph->bitmap;

        if (currentX + bitmap.width >= atlasWidth) {
            currentX = 0;
            currentY += rowHeight;
            rowHeight = 0;
        }

        for (uint32_t row = 0; row < bitmap.rows; ++row) {
            uint32_t stagingOffset = (currentY + row) * atlasWidth + currentX;
            uint32_t srcOffset = row * bitmap.pitch;
            memcpy(pData + stagingOffset, bitmap.buffer + srcOffset, bitmap.width);
        }

        glm::vec2 uvTopLeft
            = {static_cast<float>(currentX) / static_cast<float>(atlasWidth),
               static_cast<float>(currentY) / static_cast<float>(atlasHeight)};
        glm::vec2 uvBottomRight
            = {static_cast<float>(currentX + bitmap.width) / static_cast<float>(atlasWidth),
               static_cast<float>(currentY + bitmap.rows) / static_cast<float>(atlasHeight)};

        Character character
            = {uvTopLeft, uvBottomRight, glm::ivec2(bitmap.width, bitmap.rows),
               glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
               static_cast<uint32_t>(face->glyph->advance.x)};

        m_characters[c] = character;

        currentX += bitmap.width + 1;
        rowHeight = glm::max(rowHeight, bitmap.rows + 1);
    }

    auto fontTexture = std::make_unique<Texture>(
        m_device, sampler, stagingBuffer, atlasWidth, atlasHeight, VK_FORMAT_R8_UNORM);

    m_fontTextureIndex = bindlessManager.addTexture(std::move(fontTexture));
    m_lineSpacing = static_cast<float>(face->size->metrics.height >> 6);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void Text::draw(VkCommandBuffer commandBuffer, const std::vector<InstanceContent>& instances) {
    std::vector<InstanceData> outInstances;

    size_t totalChars = 0;
    for (const auto& instance : instances)
        totalChars += instance.text.length();
    outInstances.reserve(totalChars);

    for (const auto& instance : instances) {
        float x = 0.0f;
        float y = 0.0f;

        for (char c : instance.text) {
            if (c == '\n') {
                x = 0.0f;
                y += m_lineSpacing;
                continue;
            }

            if (m_characters.find(c) == m_characters.end())
                continue;
            Character ch = m_characters[c];

            float xpos = x + ch.bearing.x;
            float ypos = y - ch.bearing.y;
            float w = static_cast<float>(ch.size.x);
            float h = static_cast<float>(ch.size.y);

            InstanceData letterInstance{};
            letterInstance.worldOrigin = instance.position;
            letterInstance.offset = {xpos, ypos};
            letterInstance.size = {w, h};
            letterInstance.uvTopLeft = ch.uvTopLeft;
            letterInstance.uvBottomRight = ch.uvBottomRight;

            outInstances.push_back(letterInstance);

            x += (ch.advance >> 6);
        }
    }

    uint32_t letterCount = static_cast<uint32_t>(outInstances.size());
    if (letterCount == 0)
        return;

    if (letterCount > 10000) {
        letterCount = 10000;
    }

    memcpy(m_instanceData, outInstances.data(), sizeof(InstanceData) * letterCount);

    VkBuffer buffers[] = {m_vertexBuffer->getBuffer(), m_instanceBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0, 0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, letterCount, 0, 0, 0);
}

std::vector<VkVertexInputBindingDescription> Text::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    bindingDescriptions.push_back({1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE});

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Text::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)});

    attributeDescriptions.push_back({1, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, worldOrigin)});
    attributeDescriptions.push_back({2, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(InstanceData, offset)});
    attributeDescriptions.push_back({3, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(InstanceData, size)});
    attributeDescriptions.push_back({4, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(InstanceData, uvTopLeft)});
    attributeDescriptions.push_back({5, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(InstanceData, uvBottomRight)});

    return attributeDescriptions;
}

} // namespace gfx::billb