#include "bindless_manager.hpp"

namespace gfx {

BindlessManager::BindlessManager(vk::Device& device, vk::DescriptorSetLayout& layout, vk::DescriptorPool& pool) :
    m_device(device), m_layout(layout), m_pool(pool) {
    if (!vk::DescriptorWriter(m_layout, m_pool).build(m_descriptorSet)) {
        throw std::runtime_error("Vulkan: Failed to allocate bindless descriptor set");
    }
}

BindlessManager::~BindlessManager() {}

uint32_t BindlessManager::addTexture(std::unique_ptr<Texture> texture) {
    uint32_t index = static_cast<uint32_t>(m_textures.size());

    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView = texture->getImageView();
    info.sampler = texture->getSampler();

    m_imageInfos.push_back(info);
    m_textures.push_back(std::move(texture));

    vk::DescriptorWriter writer(m_layout, m_pool);
    writer.writeImageArray(0, m_imageInfos);
    writer.overwrite(m_descriptorSet);

    return index;
}

uint32_t BindlessManager::addCubemap(std::unique_ptr<Cubemap> cubemap) {
    uint32_t index = static_cast<uint32_t>(m_cubemaps.size());

    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView = cubemap->getImageView();
    info.sampler = cubemap->getSampler();

    m_cubemapInfos.push_back(info);
    m_cubemaps.push_back(std::move(cubemap));

    vk::DescriptorWriter writer(m_layout, m_pool);
    writer.writeImageArray(1, m_cubemapInfos);
    writer.overwrite(m_descriptorSet);

    return index;
}

void BindlessManager::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bind) {
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bind, 1, &m_descriptorSet, 0, nullptr);
}

} // namespace gfx