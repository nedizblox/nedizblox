#pragma once

#include "vk/buffer.hpp"
#include "vk/descriptors.hpp"
#include "vk/device.hpp"
#include "vk/sampler.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace gfx {

class ShadowMap {
public:
    struct UniformBufferObject {
        alignas(16) glm::mat4 lightSpace{1.0f};
        alignas(16) glm::vec4 params{0.0005f, 1.5f, 0.0f, 0.0f};
    };

    struct Settings {
        float depthBias = 0.0005f;
        float normalOffsetTexels = 1.5f;
        float strength = 0.65f;
    };

    ShadowMap(vk::Device& device, uint32_t size = 2048);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    VkRenderPass getRenderPass() const {
        return m_renderPass;
    }
    VkDescriptorSetLayout getDescriptorSetLayout() const {
        return m_setLayout->getDescriptorSetLayout();
    }
    uint32_t getSize() const {
        return m_size;
    }

    void update(
        uint32_t frameIndex, const glm::mat4& lightSpace, float orthoRadius,
        const Settings& settings
    );

    void begin(VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void end(VkCommandBuffer commandBuffer);

    void bind(
        VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
        uint32_t frameIndex, uint32_t setIndex = 1
    );

    static glm::mat4 computeLightSpace(
        const glm::vec3& toLightDir, const glm::vec3& center, float orthoRadius,
        float depthRange, uint32_t shadowMapSize
    );

private:
    struct FrameResources {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        std::unique_ptr<vk::Buffer> ubo;
        void* uboData = nullptr;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    vk::Device& m_device;
    uint32_t m_size;

    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
    bool m_linearFilter = true;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    std::unique_ptr<vk::Sampler> m_sampler;

    std::unique_ptr<vk::DescriptorSetLayout> m_setLayout;
    std::unique_ptr<vk::DescriptorPool> m_pool;

    std::vector<FrameResources> m_frames;

    void pickDepthFormat();
    void createRenderPass();
    void createSampler();
    void createDescriptorLayoutAndPool();
    void createFrameResources();
    void transitionToReadOnly();
};

} // namespace gfx