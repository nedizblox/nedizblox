#include "sampler.hpp"

#include <stdexcept>

namespace gfx::vk {

Sampler::Sampler(Device& device, const VkSamplerCreateInfo& createInfo) : m_device(device) {
    if (vkCreateSampler(m_device.getDevice(), &createInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create texture sampler");
    }
}

Sampler::~Sampler() { vkDestroySampler(m_device.getDevice(), m_sampler, nullptr); }

Sampler::Builder::Builder(Device& device) : m_device(device) {
    // default settings
    m_createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    m_createInfo.magFilter = VK_FILTER_LINEAR;
    m_createInfo.minFilter = VK_FILTER_LINEAR;
    m_createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_createInfo.anisotropyEnable = VK_FALSE;
    m_createInfo.maxAnisotropy = 1.0f;
    m_createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    m_createInfo.unnormalizedCoordinates = VK_FALSE;
    m_createInfo.compareEnable = VK_FALSE;
    m_createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    m_createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

Sampler::Builder& Sampler::Builder::setFilters(VkFilter magFilter, VkFilter minFilter) {
    m_createInfo.magFilter = magFilter;
    m_createInfo.minFilter = minFilter;
    return *this;
}

Sampler::Builder& Sampler::Builder::setAddressMode(VkSamplerAddressMode mode) {
    m_createInfo.addressModeU = mode;
    m_createInfo.addressModeV = mode;
    m_createInfo.addressModeW = mode;
    return *this;
}

Sampler::Builder& Sampler::Builder::setAnisotropy(float maxAnisotropy) {
    m_createInfo.anisotropyEnable = (maxAnisotropy > 1.0f) ? VK_TRUE : VK_FALSE;
    m_createInfo.maxAnisotropy = maxAnisotropy;
    return *this;
}

Sampler::Builder& Sampler::Builder::setMipmaps(VkSamplerMipmapMode mode, float lodBias) {
    m_createInfo.mipmapMode = mode;
    m_createInfo.mipLodBias = lodBias;
    return *this;
}

Sampler::Builder& Sampler::Builder::setMaxLod(float lod) {
    m_createInfo.minLod = 0.0f;
    m_createInfo.maxLod = lod;
    return *this;
}

std::unique_ptr<Sampler> Sampler::Builder::build() const {
    return std::make_unique<Sampler>(m_device, m_createInfo);
}

} // namespace gfx::vk