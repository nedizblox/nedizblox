#include "texture.hpp"

#include <cstring>
#include <stb/stb_image.h>
#include <stdexcept>

namespace gfx {

Texture::Texture(vk::Device& device, vk::Sampler& sampler, const std::string& imagePath, VkFormat format) :
    m_device(device), m_sampler(sampler) {
    createImage(imagePath, format);
    createImageView(format);
}

Texture::Texture(vk::Device& device, vk::Sampler& sampler, vk::Buffer& buffer, uint32_t width, uint32_t height, VkFormat format) :
    m_device(device), m_sampler(sampler) {
    createImage(buffer, width, height, format);
    createImageView(format);
}

Texture::~Texture() {
    vkDestroyImageView(m_device.getDevice(), m_imageView, nullptr);
    vmaDestroyImage(m_device.getAllocator(), m_image, m_allocation);
}

void Texture::createImage(const std::string& imagePath, VkFormat format) {
    int width, height, channels;

    stbi_uc* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        throw std::runtime_error("STB: Failed to load image");
    }

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    m_device.createImage(imageInfo, VMA_MEMORY_USAGE_AUTO, m_image, m_allocation);

    VkDeviceSize bufferSize = width * height * 4;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    stagingBuffer.uploadData(pixels, bufferSize);

    stbi_image_free(pixels);

    m_device.transitionImageLayout(
        m_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);

    m_device.copyBufferToImage(
        stagingBuffer.getBuffer(), m_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    m_device.generateMipmaps(m_image, static_cast<int32_t>(width), static_cast<int32_t>(height), m_mipLevels);
}

void Texture::createImage(vk::Buffer& buffer, uint32_t width, uint32_t height, VkFormat format) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    m_device.createImage(imageInfo, VMA_MEMORY_USAGE_AUTO, m_image, m_allocation);

    m_device.transitionImageLayout(m_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_device.copyBufferToImage(buffer.getBuffer(), m_image, width, height);
    m_device.transitionImageLayout(
        m_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Texture::createImageView(VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device.getDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create texture image view");
    }
}

} // namespace gfx