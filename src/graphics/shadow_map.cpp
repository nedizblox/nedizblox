#include "shadow_map.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstring>
#include <stdexcept>

namespace gfx {

ShadowMap::ShadowMap(vk::Device& device, uint32_t size) :
    m_device(device), m_size(size) {
    pickDepthFormat();
    createRenderPass();
    createSampler();
    createDescriptorLayoutAndPool();
    createFrameResources();
    transitionToReadOnly();
}

ShadowMap::~ShadowMap() {
    m_device.waitIdle();

    VkDevice device = m_device.getDevice();

    for (auto& frame : m_frames) {
        frame.ubo.reset();
        if (frame.framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device, frame.framebuffer, nullptr);
        if (frame.view != VK_NULL_HANDLE)
            vkDestroyImageView(device, frame.view, nullptr);
        if (frame.image != VK_NULL_HANDLE)
            vmaDestroyImage(
                m_device.getAllocator(), frame.image, frame.allocation
            );
    }

    vkDestroyRenderPass(device, m_renderPass, nullptr);
}

void ShadowMap::pickDepthFormat() {
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    const VkFormatFeatureFlags base =
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

    try {
        m_depthFormat = m_device.findSupportedFormat(
            candidates, VK_IMAGE_TILING_OPTIMAL,
            base | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
        );
        m_linearFilter = true;
    }
    catch (const std::runtime_error&) {
        m_depthFormat = m_device.findSupportedFormat(
            candidates, VK_IMAGE_TILING_OPTIMAL, base
        );
        m_linearFilter = false;
    }
}

void ShadowMap::createRenderPass() {
    VkAttachmentDescription depth{};
    depth.format = m_depthFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthRef;

    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &depth;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    info.pDependencies = dependencies.data();

    if (vkCreateRenderPass(
            m_device.getDevice(), &info, nullptr, &m_renderPass
        ) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create shadow render pass");
    }
}

void ShadowMap::createSampler() {
    VkFilter filter = m_linearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

    m_sampler = vk::Sampler::Builder(m_device)
                    .setFilters(filter, filter)
                    .setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
                    .setBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
                    .setMipmaps(VK_SAMPLER_MIPMAP_MODE_NEAREST)
                    .setMaxLod(0.0f)
                    .setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                    .build();
}

void ShadowMap::createDescriptorLayoutAndPool() {
    m_setLayout = vk::DescriptorSetLayout::Builder(m_device)
                      .addBinding(
                          0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                          VK_SHADER_STAGE_FRAGMENT_BIT
                      )
                      .addBinding(
                          1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                          VK_SHADER_STAGE_FRAGMENT_BIT
                      )
                      .build();

    const uint32_t frames =
        static_cast<uint32_t>(vk::base::MAX_FRAMES_IN_FLIGHT);

    m_pool = vk::DescriptorPool::Builder(m_device)
                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frames)
                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frames)
                 .setMaxSets(frames)
                 .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
                 .build();
}

void ShadowMap::createFrameResources() {
    const uint32_t frames =
        static_cast<uint32_t>(vk::base::MAX_FRAMES_IN_FLIGHT);
    m_frames.resize(frames);

    for (uint32_t i = 0; i < frames; i++) {
        FrameResources& frame = m_frames[i];

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {m_size, m_size, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = m_depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        m_device.createImage(
            imageInfo, VMA_MEMORY_USAGE_AUTO, frame.image, frame.allocation
        );

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = frame.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(
                m_device.getDevice(), &viewInfo, nullptr, &frame.view
            ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Vulkan: Failed to create shadow image view"
            );
        }

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &frame.view;
        fbInfo.width = m_size;
        fbInfo.height = m_size;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(
                m_device.getDevice(), &fbInfo, nullptr, &frame.framebuffer
            ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Vulkan: Failed to create shadow framebuffer"
            );
        }

        frame.ubo = std::make_unique<vk::Buffer>(
            m_device, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
        frame.uboData = frame.ubo->map();

        UniformBufferObject initial{};
        std::memcpy(frame.uboData, &initial, sizeof(UniformBufferObject));

        VkDescriptorImageInfo imageDescInfo{};
        imageDescInfo.sampler = m_sampler->getSampler();
        imageDescInfo.imageView = frame.view;
        imageDescInfo.imageLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo bufferDescInfo{};
        bufferDescInfo.buffer = frame.ubo->getBuffer();
        bufferDescInfo.offset = 0;
        bufferDescInfo.range = sizeof(UniformBufferObject);

        if (!vk::DescriptorWriter(*m_setLayout, *m_pool)
                 .writeImage(0, &imageDescInfo)
                 .writeBuffer(1, &bufferDescInfo)
                 .build(frame.descriptorSet)) {
            throw std::runtime_error(
                "Vulkan: Failed to allocate shadow descriptor set"
            );
        }
    }
}

void ShadowMap::transitionToReadOnly() {
    VkCommandBuffer commandBuffer = m_device.beginSingleTimeCommands();

    for (auto& frame : m_frames) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = frame.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &barrier
        );
    }

    m_device.endSingleTimeCommands(commandBuffer);
}

void ShadowMap::update(
    uint32_t frameIndex, const glm::mat4& lightSpace, float orthoRadius,
    const Settings& settings
) {
    UniformBufferObject ubo{};
    ubo.lightSpace = lightSpace;
    ubo.params = glm::vec4(
        settings.depthBias, settings.normalOffsetTexels,
        (2.0f * orthoRadius) / static_cast<float>(m_size), settings.strength
    );

    std::memcpy(m_frames[frameIndex].uboData, &ubo, sizeof(UniformBufferObject));
}

void ShadowMap::begin(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = m_renderPass;
    info.framebuffer = m_frames[frameIndex].framebuffer;
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = {m_size, m_size};

    VkClearValue clear{};
    clear.depthStencil = {1.0f, 0};
    info.clearValueCount = 1;
    info.pClearValues = &clear;

    vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_size);
    viewport.height = static_cast<float>(m_size);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_size, m_size};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void ShadowMap::end(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
}

void ShadowMap::bind(
    VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
    uint32_t frameIndex, uint32_t setIndex
) {
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        setIndex, 1, &m_frames[frameIndex].descriptorSet, 0, nullptr
    );
}

glm::mat4 ShadowMap::computeLightSpace(
    const glm::vec3& toLightDir, const glm::vec3& center, float orthoRadius,
    float depthRange, uint32_t shadowMapSize
) {
    glm::vec3 dir = glm::normalize(toLightDir);
    glm::vec3 up = glm::abs(dir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                           : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 eye = center + dir * depthRange;
    glm::mat4 view = glm::lookAtRH(eye, center, up);
    glm::mat4 proj = glm::orthoRH_ZO(
        -orthoRadius, orthoRadius, -orthoRadius, orthoRadius, 0.0f,
        depthRange * 2.0f
    );

    glm::vec4 origin = proj * view * glm::vec4(center, 1.0f);
    float halfSize = static_cast<float>(shadowMapSize) * 0.5f;
    glm::vec2 scaled = glm::vec2(origin.x, origin.y) * halfSize;
    glm::vec2 rounded = glm::round(scaled);
    glm::vec2 offset = (rounded - scaled) / halfSize;

    proj[3][0] += offset.x;
    proj[3][1] += offset.y;

    return proj * view;
}

} // namespace gfx