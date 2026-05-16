#pragma once

#include "device.hpp"

#include <memory>

namespace gfx::vk {

class Sampler {
public:
    class Builder {
    public:
        Builder(Device& device);

        Builder& setFilters(VkFilter magFilter, VkFilter minFilter);
        Builder& setAddressMode(VkSamplerAddressMode mode);
        Builder& setAnisotropy(float maxAnisotropy);
        Builder& setMipmaps(VkSamplerMipmapMode mode, float lodBias = 0.0f);
        Builder& setMaxLod(float lod);

        std::unique_ptr<Sampler> build() const;

    private:
        Device& m_device;

        VkSamplerCreateInfo m_createInfo{};
    };

    Sampler(Device& device, const VkSamplerCreateInfo& createInfo);
    ~Sampler();

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    VkSampler getSampler() const { return m_sampler; }

private:
    Device& m_device;

    VkSampler m_sampler;
};

} // namespace gfx::vk