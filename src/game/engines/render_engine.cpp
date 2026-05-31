#include "render_engine.hpp"

#include <format>

namespace game::engines {

RenderEngine::RenderEngine(gfx::vk::Device& device, gfx::vk::Renderer& renderer, gfx::mngrs::BindlessManager& bindlessManager) :
    m_device(device), m_renderer(renderer), m_bindlessManager(bindlessManager) {}

RenderEngine::~RenderEngine() {}

void RenderEngine::initPipelines(VkDescriptorSetLayout setLayout) {
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

    m_pipelines["modelOpaque"]
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

    m_pipelines["modelTransparent"]
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
}

void RenderEngine::renderModelsOpaque(
    VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
    gfx::mngrs::ModelManager& modelManager,
    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["modelOpaque"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Model::PushConstantObject modelPush{};
    modelPush.viewProj = camera.getProjection() * camera.getView();
    pipeline->pushConstant(commandBuffer, modelPush);

    modelManager.drawOpaque(commandBuffer, instancesData);
}

void RenderEngine::renderSkybox(
    VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera, gfx::Skybox& skybox,
    uint32_t skyboxCubemapId) {
    auto& pipeline = m_pipelines["skybox"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Skybox::PushConstantObject skyboxPush{};
    skyboxPush.viewProj = camera.getProjection() * glm::mat4(glm::mat3(camera.getView()));
    skyboxPush.cubIndex = skyboxCubemapId;
    pipeline->pushConstant(commandBuffer, skyboxPush);

    skybox.draw(commandBuffer);
}

void RenderEngine::renderModelsTransparent(
    VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
    gfx::mngrs::ModelManager& modelManager,
    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["modelTransparent"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Model::PushConstantObject modelPush{};
    modelPush.viewProj = camera.getProjection() * camera.getView();
    pipeline->pushConstant(commandBuffer, modelPush);

    modelManager.drawTransparent(commandBuffer, instancesData);
}

void RenderEngine::renderDebugUI(
    VkCommandBuffer cmd, uint32_t windowWidth, uint32_t windowHeight, gfx::ui::Text& fpsFont, float deltaTime) {
    auto& pipeline = m_pipelines["text"];
    pipeline->bind(cmd);
    m_bindlessManager.bind(cmd, pipeline->getPipelineLayout());

    gfx::ui::Text::PushConstantObject textPush{};
    textPush.proj = glm::ortho(0.0f, static_cast<float>(windowWidth), 0.0f, static_cast<float>(windowHeight));
    textPush.texIndex = fpsFont.getTextureIndex();
    textPush.scale = glm::vec2(0.5f);

    float fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
    fpsFont.setText(std::format("FPS: {:.0f}\nTest\nTest", fps), glm::vec2(20.0f, 40.0f));

    pipeline->pushConstant(cmd, textPush);
    fpsFont.draw(cmd);
}

} // namespace game::engines