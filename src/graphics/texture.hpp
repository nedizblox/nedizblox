#pragma once

#include "vk/buffer.hpp"
#include "vk/device.hpp"
#include "vk/sampler.hpp"

#include <string>

namespace gfx {

class Texture {
public:
    Texture(
        vk::Device& device, vk::Sampler& sampler, const std::string& imagePath,
        bool flipVertically = false, bool genMipMaps = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    Texture(vk::Device& device, vk::Sampler& sampler, vk::Buffer& buffer, uint32_t width, uint32_t height, VkFormat format);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler.getSampler(); }

private:
    vk::Device& m_device;

    VmaAllocation m_allocation;

    VkImage m_image;
    VkImageView m_imageView;
    vk::Sampler& m_sampler;

    uint32_t m_mipLevels = 1;

    void createImage(const std::string& imagePath, VkFormat format, bool flipVertically, bool genMipMaps);
    void createImage(vk::Buffer& buffer, uint32_t width, uint32_t height, VkFormat format);
    void createImageView(VkFormat format);
};

} // namespace gfx