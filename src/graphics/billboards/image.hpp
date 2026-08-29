#pragma once

#include "../vk/buffer.hpp"
#include "../vk/device.hpp"

#include "../managers/bindless_manager.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

namespace gfx::billb {

class Image {
public:
    struct Vertex {
        glm::vec2 position{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 view{1.0f};
        alignas(16) glm::mat4 proj{1.0f};
        alignas(4) uint32_t texIndex{0};
    };

    struct InstanceData {
        glm::vec3 position;
        glm::vec2 scale;
    };

    Image(vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager, const std::string& imagePath);
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    uint32_t getTextureIndex() const { return m_textureIndex; }

    void draw(VkCommandBuffer commandBuffer, const std::vector<InstanceData>& instances);

private:
    vk::Device& m_device;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    std::unique_ptr<vk::Buffer> m_indexBuffer;

    std::unique_ptr<vk::Buffer> m_instanceBuffer;
    void* m_instanceData;

    uint32_t m_textureIndex;

    void createVertexBuffer();
    void createIndexBuffer();
    void createInstanceBuffer();
    void loadImage(const std::string& imagePath, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager);
};

} // namespace gfx::billb