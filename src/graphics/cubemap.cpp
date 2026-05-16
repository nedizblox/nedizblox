#include "cubemap.hpp"
#include "vk/buffer.hpp"

#include <cstring>
#include <stb/stb_image.h>
#include <stdexcept>
#include <vector>

namespace gfx {

Cubemap::Cubemap(vk::Device& device, vk::Sampler& sampler, const std::array<std::string, 6>& faces) :
    m_device(device), m_sampler(sampler) {
    createImage(faces);
    createImageView();
}

Cubemap::~Cubemap() {
    vkDestroyImageView(m_device.getDevice(), m_imageView, nullptr);
    vmaDestroyImage(m_device.getAllocator(), m_image, m_allocation);
}

void Cubemap::createImage(const std::array<std::string, 6>& faces) {
    int width, height, channels;
    std::vector<stbi_uc*> images(6);

    for (size_t i = 0; i < 6; i++) {
        int w, h, c;
        images[i] = stbi_load(faces[i].c_str(), &w, &h, &c, 4);
        if (!images[i]) {
            for (int j = 0; j < i; j++)
                stbi_image_free(images[j]);
            throw std::runtime_error("STB: Failed to load texture image file: " + faces[i]);
        }

        if (i == 0) {
            width = w;
            height = h;
        } else if (w != width || h != height) {
            for (int j = 0; j <= i; j++)
                stbi_image_free(images[j]);
            throw std::runtime_error("Textures must have identical width and height");
        }
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    m_device.createImage(imageInfo, VMA_MEMORY_USAGE_AUTO, m_image, m_allocation);

    VkDeviceSize layerSize = width * height * 4;
    VkDeviceSize totalSize = layerSize * 6;

    vk::Buffer stagingBuffer(
        m_device, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    void* data = stagingBuffer.map();
    for (size_t i = 0; i < 6; i++) {
        memcpy(static_cast<uint8_t*>(data) + (i * layerSize), images[i], static_cast<size_t>(layerSize));
        stbi_image_free(images[i]);
    }
    stagingBuffer.unmap();

    m_device.transitionImageLayout(
        m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 6);
    m_device.copyBufferToImage(
        stagingBuffer.getBuffer(), m_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 6);
    m_device.transitionImageLayout(
        m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);
}

void Cubemap::createImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(m_device.getDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create texture image view");
    }
}

} // namespace gfx