#pragma once

#include "../vk/buffer.hpp"
#include "../vk/device.hpp"

#include "../managers/bindless_manager.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace gfx::ui {

class Text {
public:
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 proj{1.0f};
        alignas(8) glm::vec2 scale{1.0f};
        alignas(4) uint32_t texIndex{0};
    };

    Text(vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager, const std::string& fontPath, uint32_t maxChars);
    ~Text();

    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;

    uint32_t getTextureIndex() const { return m_fontTextureIndex; }

    void setText(const std::string& text, const glm::vec2& position);

    void draw(VkCommandBuffer commandBuffer);

private:
    struct Character {
        glm::vec2 uvTopLeft;
        glm::vec2 uvBottomRight;

        glm::ivec2 size;
        glm::ivec2 bearing;
        uint32_t advance;
    };

    vk::Device& m_device;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    uint32_t m_vertexCount;
    void* m_vertexData;

    std::unique_ptr<vk::Buffer> m_indexBuffer;
    uint32_t m_indexCount;

    std::unordered_map<char, Character> m_characters;
    uint32_t m_fontTextureIndex;
    float m_lineSpacing;

    void createVertexBuffer(uint32_t maxChars);
    void createIndexBuffer(uint32_t maxChars);
    void loadFont(const std::string& fontPath, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager);
};

} // namespace gfx::ui