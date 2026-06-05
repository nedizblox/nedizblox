#pragma once

#include "../vk/buffer.hpp"
#include "../vk/device.hpp"

#include "../managers/bindless_manager.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace gfx::billb {

class Text {
public:
    struct Vertex {
        glm::vec2 position;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 viewProj{1.0f};
        alignas(8) glm::vec2 scale{1.0f};
        alignas(4) uint32_t texIndex{0};
    };

    struct InstanceData {
        glm::vec3 worldOrigin;
        glm::vec2 offset;
        glm::vec2 size;
        glm::vec2 uvTopLeft;
        glm::vec2 uvBottomRight;
    };

    struct InstanceContent {
        std::string text;
        glm::vec3 position;
    };

    Text(vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager, const std::string& fontPath, uint32_t maxChars);
    ~Text();

    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;

    uint32_t getTextureIndex() const { return m_fontTextureIndex; }

    void draw(VkCommandBuffer commandBuffer, const std::vector<InstanceContent>& instances);

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

    std::unique_ptr<vk::Buffer> m_indexBuffer;

    std::unique_ptr<vk::Buffer> m_instanceBuffer;
    void* m_instanceData;

    std::unordered_map<char, Character> m_characters;
    uint32_t m_fontTextureIndex;
    float m_lineSpacing;

    void createVertexBuffer();
    void createIndexBuffer(uint32_t maxChars);
    void createInstanceBuffer();
    void loadFont(const std::string& fontPath, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager);
};

} // namespace gfx::billb