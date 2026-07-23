#pragma once

#include "vk/buffer.hpp"
#include "vk/device.hpp"

#include <memory>
#include <vector>

namespace gfx {

class Skybox {
public:
    struct Vertex {
        glm::vec3 position{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 viewProj{1.0f};
        alignas(4) uint32_t cubIndex{0};
    };

    Skybox(vk::Device& device);
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void draw(VkCommandBuffer commandBuffer);

private:
    vk::Device& m_device;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    uint32_t m_vertexCount;

    std::unique_ptr<vk::Buffer> m_indexBuffer;
    uint32_t m_indexCount;

    void createVertexBuffers(const std::vector<Vertex>& vertices);
    void createIndexBuffers(const std::vector<uint32_t>& indices);
};

} // namespace gfx