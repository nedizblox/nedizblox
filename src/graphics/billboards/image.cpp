#include "image.hpp"

#include <stb/stb_image.h>

#include <cstring>
#include <stdexcept>

namespace gfx::billb {

Image::Image(vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager, const std::string& imagePath) :
    m_device(device) {
    createVertexBuffer();
    createIndexBuffer();
    createInstanceBuffer();
    loadImage(imagePath, sampler, bindlessManager);
}

Image::~Image() {}

void Image::createVertexBuffer() {
    std::vector<glm::vec2> vertices = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(vertices.data(), bufferSize);

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
}

void Image::createIndexBuffer() {
    std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(indices.data(), bufferSize);

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
}

void Image::createInstanceBuffer() {
    VkDeviceSize bufferSize = sizeof(InstanceData) * 10000;

    m_instanceBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    m_instanceData = m_instanceBuffer->map();
}

void Image::loadImage(const std::string& imagePath, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager) {
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;

    stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("stb_image: Failed to load image at " + imagePath);
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;

    vk::Buffer stagingBuffer(
        m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    stagingBuffer.uploadData(pixels, imageSize);
    stbi_image_free(pixels);

    auto texture = std::make_unique<Texture>(
        m_device, sampler, stagingBuffer, static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight), VK_FORMAT_R8G8B8A8_SRGB);

    m_textureIndex = bindlessManager.addTexture(std::move(texture));
}

void Image::draw(VkCommandBuffer commandBuffer, const std::vector<InstanceData>& instances) {
    if (instances.empty())
        return;

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    std::memcpy(m_instanceData, instances.data(), sizeof(InstanceData) * instanceCount);

    VkBuffer buffers[] = {m_vertexBuffer->getBuffer(), m_instanceBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0, 0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, instanceCount, 0, 0, 0);
}

std::vector<VkVertexInputBindingDescription> Image::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    bindingDescriptions.push_back({1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE});

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Image::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)});

    attributeDescriptions.push_back({1, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, position)});
    attributeDescriptions.push_back({2, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(InstanceData, scale)});

    return attributeDescriptions;
}

} // namespace gfx::billb