#include "skybox.hpp"

namespace gfx {

Skybox::Skybox(vk::Device& device) : m_device(device) {
    createVertexBuffers(
        {{{-1.0f, -1.0f, -1.0f}},
         {{1.0f, -1.0f, -1.0f}},
         {{1.0f, 1.0f, -1.0f}},
         {{-1.0f, 1.0f, -1.0f}},

         {{-1.0f, -1.0f, 1.0f}},
         {{1.0f, -1.0f, 1.0f}},
         {{1.0f, 1.0f, 1.0f}},
         {{-1.0f, 1.0f, 1.0f}}});

    createIndexBuffers({0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 3, 2, 6, 6, 7, 3,
                        0, 1, 5, 5, 4, 0, 0, 3, 7, 7, 4, 0, 1, 2, 6, 6, 5, 1});
}

Skybox::~Skybox() {} // buffers will be automatically destroyed

void Skybox::createVertexBuffers(const std::vector<Vertex>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());

    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(vertices.data(), bufferSize);

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
}

void Skybox::createIndexBuffers(const std::vector<uint32_t>& indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(indices.data(), bufferSize);

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
}

void Skybox::draw(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
}

std::vector<VkVertexInputBindingDescription> Skybox::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Skybox::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});

    return attributeDescriptions;
}

} // namespace gfx