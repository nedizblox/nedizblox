#pragma once

#include "device.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gfx::vk {

class Pipeline {
public:
    struct PipelineConfigInfo {
        std::string vertPath;
        std::string fragPath;
        std::vector<VkDescriptorSetLayout> descriptorLayouts{};
        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        std::vector<VkDynamicState> dynamicStateEnables{};
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t constantSize = 0;
        uint32_t subpass = 0;
    };

    class Builder {
    public:
        Builder(Device& device);

        Builder& setVertShaderPath(const std::string& path);
        Builder& setFragShaderPath(const std::string& path);

        Builder& setRenderPass(VkRenderPass renderPass);
        Builder& setDescriptorLayouts(std::vector<VkDescriptorSetLayout> layouts);
        Builder& setConstantSize(size_t size);

        Builder& setBindingDescriptions(std::vector<VkVertexInputBindingDescription> bindings);
        Builder& setAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributes);

        Builder& enableDepthTest();
        Builder& disableDepthWrite();
        Builder& setDepthCompareOp(VkCompareOp op);

        Builder& setDepthBias(float constantFactor, float clamp, float slopeFactor);

        Builder& setCullMode(VkCullModeFlags mode);

        Builder& enableAlphaBlending();

        std::unique_ptr<Pipeline> build() const;

    private:
        Device& m_device;

        PipelineConfigInfo m_configInfo{};
    };

    Pipeline(Device& device, const PipelineConfigInfo& config);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

    void bind(VkCommandBuffer commandBuffer);

    template <typename T>
    void pushConstant(VkCommandBuffer commandBuffer, const T& data) {
        vkCmdPushConstants(
            commandBuffer, m_pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(T), &data);
    }

private:
    Device& m_device;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    static std::vector<char> readFile(const std::string& filepath);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    void createPipelineLayout(const PipelineConfigInfo& configInfo);
    void createGraphicsPipeline(const PipelineConfigInfo& configInfo);
};

} // namespace gfx::vk