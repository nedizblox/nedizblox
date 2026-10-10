#include "pipeline_manager.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace gfx::mngrs {

PipelineManager::PipelineManager(
    gfx::vk::Device& device, gfx::vk::Renderer& renderer,
    gfx::mngrs::BindlessManager& bindlessManager, gfx::mngrs::ModelManager& modelManager,
    gfx::mngrs::BillboardManager& billboardManager, gfx::mngrs::UiManager& uiManager) :
    m_device(device),
    m_renderer(renderer),
    m_bindlessManager(bindlessManager),
    m_modelManager(modelManager),
    m_billboardManager(billboardManager),
    m_uiManager(uiManager) {}

PipelineManager::~PipelineManager() {}

void PipelineManager::initPipelines(VkDescriptorSetLayout setLayout) {
    m_pipelines["skybox"] = gfx::vk::Pipeline::Builder(m_device)
                                .setVertShaderPath("shaders/skybox.vert.spv")
                                .setFragShaderPath("shaders/skybox.frag.spv")
                                .setConstantSize(sizeof(gfx::Skybox::PushConstantObject))
                                .setDescriptorLayouts({setLayout})
                                .setRenderPass(m_renderer.getRenderPass())
                                .setBindingDescriptions(gfx::Skybox::Vertex::getBindingDescriptions())
                                .setAttributeDescriptions(gfx::Skybox::Vertex::getAttributeDescriptions())
                                .enableDepthTest()
                                .disableDepthWrite()
                                .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                                .build();

    m_pipelines["model_opaque"]
        = gfx::vk::Pipeline::Builder(m_device)
              .setVertShaderPath("shaders/model.vert.spv")
              .setFragShaderPath("shaders/model.frag.spv")
              .setConstantSize(sizeof(gfx::Model::PushConstantObject))
              .setDescriptorLayouts({setLayout})
              .setRenderPass(m_renderer.getRenderPass())
              .setBindingDescriptions(gfx::Model::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Model::Vertex::getAttributeDescriptions())
              .setCullMode(VK_CULL_MODE_FRONT_BIT)
              .enableDepthTest()
              .build();

    m_pipelines["model_transparent"]
        = gfx::vk::Pipeline::Builder(m_device)
              .setVertShaderPath("shaders/model.vert.spv")
              .setFragShaderPath("shaders/model.frag.spv")
              .setConstantSize(sizeof(gfx::Model::PushConstantObject))
              .setDescriptorLayouts({setLayout})
              .setRenderPass(m_renderer.getRenderPass())
              .setBindingDescriptions(gfx::Model::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Model::Vertex::getAttributeDescriptions())
              .setCullMode(VK_CULL_MODE_FRONT_BIT)
              .enableDepthTest()
              .disableDepthWrite()
              .enableAlphaBlending()
              .build();

    m_pipelines["model_outline_opaque"]
        = gfx::vk::Pipeline::Builder(m_device)
              .setVertShaderPath("shaders/model_outline.vert.spv")
              .setFragShaderPath("shaders/model_outline.frag.spv")
              .setConstantSize(sizeof(gfx::Model::PushConstantObject))
              .setDescriptorLayouts({setLayout})
              .setRenderPass(m_renderer.getRenderPass())
              .setBindingDescriptions(gfx::Model::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Model::Vertex::getAttributeDescriptions())
              .enableDepthTest()
              .setDepthBias(-2.0f, 0.0f, -2.0f)
              .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
              .build();

    m_pipelines["model_outline_transparent"]
        = gfx::vk::Pipeline::Builder(m_device)
              .setVertShaderPath("shaders/model_outline.vert.spv")
              .setFragShaderPath("shaders/model_outline.frag.spv")
              .setConstantSize(sizeof(gfx::Model::PushConstantObject))
              .setDescriptorLayouts({setLayout})
              .setRenderPass(m_renderer.getRenderPass())
              .setBindingDescriptions(gfx::Model::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Model::Vertex::getAttributeDescriptions())
              .enableDepthTest()
              .disableDepthWrite()
              .setDepthBias(-2.0f, 0.0f, -2.0f)
              .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
              .enableAlphaBlending()
              .build();

    m_pipelines["text"] = gfx::vk::Pipeline::Builder(m_device)
                              .setVertShaderPath("shaders/text.vert.spv")
                              .setFragShaderPath("shaders/text.frag.spv")
                              .setConstantSize(sizeof(gfx::ui::Text::PushConstantObject))
                              .setDescriptorLayouts({setLayout})
                              .setRenderPass(m_renderer.getRenderPass())
                              .setBindingDescriptions(gfx::ui::Text::Vertex::getBindingDescriptions())
                              .setAttributeDescriptions(gfx::ui::Text::Vertex::getAttributeDescriptions())
                              .enableAlphaBlending()
                              .build();

    m_pipelines["billboard_text"]
        = gfx::vk::Pipeline::Builder(m_device)
              .setVertShaderPath("shaders/billboard_text.vert.spv")
              .setFragShaderPath("shaders/text.frag.spv")
              .setConstantSize(sizeof(gfx::billb::Text::PushConstantObject))
              .setDescriptorLayouts({setLayout})
              .setRenderPass(m_renderer.getRenderPass())
              .setBindingDescriptions(gfx::billb::Text::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::billb::Text::Vertex::getAttributeDescriptions())
              .enableDepthTest()
              .disableDepthWrite()
              .enableAlphaBlending()
              .build();

    m_pipelines["imgui"] = gfx::vk::Pipeline::Builder(m_device)
                               .setVertShaderPath("shaders/imgui.vert.spv")
                               .setFragShaderPath("shaders/imgui.frag.spv")
                               .setConstantSize(sizeof(gfx::ui::Imgui::PushConstantObject))
                               .setDescriptorLayouts({setLayout})
                               .setRenderPass(m_renderer.getRenderPass())
                               .setBindingDescriptions(gfx::ui::Imgui::Vertex::getBindingDescriptions())
                               .setAttributeDescriptions(gfx::ui::Imgui::Vertex::getAttributeDescriptions())
                               .enableAlphaBlending()
                               .build();
}

void PipelineManager::renderModelsOpaque(
    VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, const glm::vec3& pos,
    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["model_opaque"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Model::PushConstantObject push{};
    push.viewProj = proj * view;
    push.cameraPos = pos;
    pipeline->pushConstant(commandBuffer, push);

    m_modelManager.drawOpaque(commandBuffer, instancesData);
}

void PipelineManager::renderModelOutlinesOpaque(
    VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, const glm::vec3& pos,
    const std::unordered_map<std::string, std::vector<gfx::ModelOutline::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["model_outline_opaque"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::ModelOutline::PushConstantObject push{};
    push.viewProj = proj * view;
    push.cameraPos = pos;
    pipeline->pushConstant(commandBuffer, push);

    m_modelManager.drawOutlinesOpaque(commandBuffer, instancesData);
}

void PipelineManager::renderSkybox(VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, uint32_t skyboxCubId) {
    auto& pipeline = m_pipelines["skybox"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Skybox::PushConstantObject push{};
    push.viewProj = proj * glm::mat4(glm::mat3(view));
    push.cubIndex = skyboxCubId;
    pipeline->pushConstant(commandBuffer, push);

    m_modelManager.drawSkybox(commandBuffer);
}

void PipelineManager::renderModelsTransparent(
    VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, const glm::vec3& pos,
    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["model_transparent"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Model::PushConstantObject push{};
    push.viewProj = proj * view;
    push.cameraPos = pos;
    pipeline->pushConstant(commandBuffer, push);

    m_modelManager.drawTransparent(commandBuffer, instancesData);
}

void PipelineManager::renderModelOutlinesTransparent(
    VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, const glm::vec3& pos,
    const std::unordered_map<std::string, std::vector<gfx::ModelOutline::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["model_outline_transparent"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::ModelOutline::PushConstantObject push{};
    push.viewProj = proj * view;
    push.cameraPos = pos;
    pipeline->pushConstant(commandBuffer, push);

    m_modelManager.drawOutlinesTransparent(commandBuffer, instancesData);
}

void PipelineManager::renderBillboardTexts(
    VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view,
    const std::unordered_map<std::string, std::vector<gfx::billb::Text::InstanceContent>>& instancesData) {
    auto& pipeline = m_pipelines["billboard_text"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    for (const auto& [name, instances] : instancesData) {
        if (instances.empty())
            continue;

        gfx::billb::Text::PushConstantObject push{};
        push.view = view;
        push.proj = proj;
        push.texIndex = m_billboardManager.getTextureIndex(name);

        pipeline->pushConstant(commandBuffer, push);
        m_billboardManager.drawText(commandBuffer, instances);
    }
}

void PipelineManager::renderTexts(
    VkCommandBuffer commandBuffer, win::Window& window,
    const std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>>& instancesData) {
    auto& pipeline = m_pipelines["text"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    for (const auto& [name, instances] : instancesData) {
        if (instances.empty())
            continue;

        gfx::ui::Text::PushConstantObject push{};
        push.proj = glm::ortho(
            0.0f, static_cast<float>(window.getWidth()), 0.0f, static_cast<float>(window.getHeight()));
        push.texIndex = m_uiManager.getTextureIndex(name);

        pipeline->pushConstant(commandBuffer, push);
        m_uiManager.drawText(commandBuffer, instances);
    }
}

void PipelineManager::renderImgui(VkCommandBuffer commandBuffer, win::Window& window) {
    auto& pipeline = m_pipelines["imgui"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::ui::Imgui::PushConstantObject push{};
    push.proj = glm::ortho(
        0.0f, static_cast<float>(window.getWidth()), 0.0f, static_cast<float>(window.getHeight()));

    pipeline->pushConstant(commandBuffer, push);
    m_uiManager.drawImgui(commandBuffer);
}

} // namespace gfx::mngrs