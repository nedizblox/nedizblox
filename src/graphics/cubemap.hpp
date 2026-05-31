#pragma once

#include "vk/device.hpp"
#include "vk/sampler.hpp"

#include <array>
#include <string>

namespace gfx {

class Cubemap {
public:
    Cubemap(vk::Device& device, vk::Sampler& sampler, const std::array<std::string, 6>& faces, bool flipVertically = false);
    ~Cubemap();

    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler.getSampler(); }

private:
    vk::Device& m_device;

    VmaAllocation m_allocation;

    VkImage m_image;
    VkImageView m_imageView;
    vk::Sampler& m_sampler;

    void createImage(const std::array<std::string, 6>& faces, bool flipVertically);
    void createImageView();
};

} // namespace gfx