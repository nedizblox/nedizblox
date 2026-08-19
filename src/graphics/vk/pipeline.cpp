#include "pipeline.hpp"
#include "../model.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>

namespace gfx::vk {

Pipeline::Pipeline(Device& device, const PipelineConfigInfo& configInfo) : m_device(device) {
    createPipelineLayout(configInfo);
    createGraphicsPipeline(configInfo);
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(m_device.getDevice(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device.getDevice(), m_pipelineLayout, nullptr);
}

Pipeline::Builder::Builder(Device& device) : m_device(device) {
    // default settings
    m_configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    m_configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_configInfo.viewportInfo.viewportCount = 1;
    m_configInfo.viewportInfo.pViewports = nullptr;
    m_configInfo.viewportInfo.scissorCount = 1;
    m_configInfo.viewportInfo.pScissors = nullptr;

    m_configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    m_configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    m_configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    m_configInfo.rasterizationInfo.lineWidth = 1.0f;
    m_configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    m_configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    m_configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
    m_configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
    m_configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
    m_configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

    m_configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    m_configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_configInfo.multisampleInfo.minSampleShading = 1.0f;
    m_configInfo.multisampleInfo.pSampleMask = nullptr;
    m_configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    m_configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

    m_configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                                       | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    m_configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    m_configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    m_configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    m_configInfo.colorBlendInfo.attachmentCount = 1;
    m_configInfo.colorBlendInfo.pAttachments = &m_configInfo.colorBlendAttachment;
    m_configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
    m_configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
    m_configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
    m_configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

    m_configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_configInfo.depthStencilInfo.depthTestEnable = VK_FALSE;
    m_configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
    m_configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    m_configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    m_configInfo.depthStencilInfo.minDepthBounds = 0.0f;
    m_configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
    m_configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    m_configInfo.depthStencilInfo.front = {};
    m_configInfo.depthStencilInfo.back = {};

    m_configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    m_configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_configInfo.dynamicStateInfo.pDynamicStates = m_configInfo.dynamicStateEnables.data();
    m_configInfo.dynamicStateInfo.dynamicStateCount
        = static_cast<uint32_t>(m_configInfo.dynamicStateEnables.size());
    m_configInfo.dynamicStateInfo.flags = 0;

    m_configInfo.bindingDescriptions = Model::Vertex::getBindingDescriptions();
    m_configInfo.attributeDescriptions = Model::Vertex::getAttributeDescriptions();
}

Pipeline::Builder& Pipeline::Builder::setVertShaderPath(const std::string& path) {
    m_configInfo.vertPath = path;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setFragShaderPath(const std::string& path) {
    m_configInfo.fragPath = path;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setRenderPass(VkRenderPass renderPass) {
    m_configInfo.renderPass = renderPass;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setDescriptorLayouts(std::vector<VkDescriptorSetLayout> layouts) {
    m_configInfo.descriptorLayouts = layouts;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setConstantSize(size_t size) {
    m_configInfo.constantSize = static_cast<uint32_t>(size);
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setBindingDescriptions(std::vector<VkVertexInputBindingDescription> bindings) {
    m_configInfo.bindingDescriptions = bindings;
    return *this;
}

Pipeline::Builder&
Pipeline::Builder::setAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributes) {
    m_configInfo.attributeDescriptions = attributes;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::enableDepthTest() {
    m_configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    m_configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    m_configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::disableDepthWrite() {
    m_configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setDepthCompareOp(VkCompareOp op) {
    m_configInfo.depthStencilInfo.depthCompareOp = op;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setDepthBias(float constantFactor, float clamp, float slopeFactor) {
    m_configInfo.rasterizationInfo.depthBiasEnable = VK_TRUE;
    m_configInfo.rasterizationInfo.depthBiasConstantFactor = constantFactor;
    m_configInfo.rasterizationInfo.depthBiasClamp = clamp;
    m_configInfo.rasterizationInfo.depthBiasConstantFactor = slopeFactor;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::setCullMode(VkCullModeFlags mode) {
    m_configInfo.rasterizationInfo.cullMode = mode;
    return *this;
}

Pipeline::Builder& Pipeline::Builder::enableAlphaBlending() {
    m_configInfo.colorBlendAttachment.blendEnable = VK_TRUE;

    m_configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    m_configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    return *this;
}

std::unique_ptr<Pipeline> Pipeline::Builder::build() const {
    assert(!m_configInfo.vertPath.empty() && !m_configInfo.fragPath.empty());
    assert(m_configInfo.renderPass != VK_NULL_HANDLE);
    assert(m_configInfo.constantSize != 0);
    return std::make_unique<Pipeline>(m_device, m_configInfo);
}

void Pipeline::createPipelineLayout(const PipelineConfigInfo& configInfo) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = configInfo.constantSize;

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = static_cast<uint32_t>(configInfo.descriptorLayouts.size());
    createInfo.pSetLayouts = configInfo.descriptorLayouts.data();
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device.getDevice(), &createInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create pipeline layout");
    }
}

void Pipeline::createGraphicsPipeline(const PipelineConfigInfo& configInfo) {
    std::vector<char> vertCode = readFile(configInfo.vertPath);
    std::vector<char> fragCode = readFile(configInfo.fragPath);

    VkShaderModule vertShaderModule = createShaderModule(vertCode);
    VkShaderModule fragShaderModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount
        = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount
        = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeDescriptions.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &configInfo.viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = configInfo.renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline)
        != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create graphics pipeline");
    }

    // after creating the graphics pipeline, shader modules are not needed
    vkDestroyShaderModule(m_device.getDevice(), vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device.getDevice(), fragShaderModule, nullptr);
}

std::vector<char> Pipeline::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Filesystem: Failed to open file: " + filepath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule Pipeline::createShaderModule(const std::vector<char>& code) {
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(m_device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create shader module");
    }

    return shaderModule;
}

void Pipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
}

} // namespace gfx::vk