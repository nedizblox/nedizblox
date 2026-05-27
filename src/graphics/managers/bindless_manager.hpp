#pragma once

#include "../vk/descriptors.hpp"
#include "../vk/device.hpp"

#include "../cubemap.hpp"
#include "../texture.hpp"

#include <memory>
#include <vector>

namespace gfx::mngrs {

class BindlessManager {
public:
    BindlessManager(vk::Device& device, vk::DescriptorSetLayout& layout, vk::DescriptorPool& pool);
    ~BindlessManager();

    BindlessManager(const BindlessManager&) = delete;
    BindlessManager& operator=(const BindlessManager&) = delete;

    uint32_t addTexture(std::unique_ptr<Texture> texture);
    uint32_t addCubemap(std::unique_ptr<Cubemap> cubemap);

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bind = 0);

private:
    vk::Device& m_device;
    vk::DescriptorSetLayout& m_layout;
    vk::DescriptorPool& m_pool;

    VkDescriptorSet m_descriptorSet;

    std::vector<VkDescriptorImageInfo> m_imageInfos;
    std::vector<VkDescriptorImageInfo> m_cubemapInfos;

    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<Cubemap>> m_cubemaps;
};

} // namespace gfx::mngrs