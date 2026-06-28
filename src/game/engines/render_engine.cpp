#include "render_engine.hpp"

#include <glm/gtc/matrix_transform.hpp>

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

    m_pipelines["billboard"]
        = gfx::vk::Pipeline::Builder(m_device)
              .setVertShaderPath("shaders/billboard.vert.spv")
              .setFragShaderPath("shaders/text.frag.spv")
              .setConstantSize(sizeof(gfx::Billboard::PushConstantObject))
              .setDescriptorLayouts({setLayout})
              .setRenderPass(m_renderer.getRenderPass())
              .setBindingDescriptions(gfx::Billboard::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Billboard::Vertex::getAttributeDescriptions())
              .enableDepthTest()
              .disableDepthWrite()
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

    gfx::Model::PushConstantObject push{};
    push.viewProj = camera.getProjection() * camera.getView();
    push.cameraPos = camera.getPosition();
    pipeline->pushConstant(commandBuffer, push);

    modelManager.drawOpaque(commandBuffer, instancesData);
}

void RenderEngine::renderSkybox(
    VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera, gfx::Skybox& skybox,
    uint32_t skyboxCubemapId) {
    auto& pipeline = m_pipelines["skybox"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Skybox::PushConstantObject push{};
    push.viewProj = camera.getProjection() * glm::mat4(glm::mat3(camera.getView()));
    push.cubIndex = skyboxCubemapId;
    pipeline->pushConstant(commandBuffer, push);

    skybox.draw(commandBuffer);
}

void RenderEngine::renderModelsTransparent(
    VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
    gfx::mngrs::ModelManager& modelManager,
    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData) {
    auto& pipeline = m_pipelines["modelTransparent"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    gfx::Model::PushConstantObject push{};
    push.viewProj = camera.getProjection() * camera.getView();
    push.cameraPos = camera.getPosition();
    pipeline->pushConstant(commandBuffer, push);

    modelManager.drawTransparent(commandBuffer, instancesData);
}

void RenderEngine::renderBillboardTexts(
    VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
    gfx::mngrs::BillboardManager& billboardManager,
    const std::unordered_map<std::string, std::vector<gfx::Billboard::InstanceContent>>& instancesData) {
    auto& pipeline = m_pipelines["billboard"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    for (const auto& [name, instances] : instancesData) {
        if (instances.empty())
            continue;

        gfx::Billboard::PushConstantObject push{};
        push.view = camera.getView();
        push.proj = camera.getProjection();
        push.texIndex = billboardManager.getTextureIndex(name);

        pipeline->pushConstant(commandBuffer, push);
        billboardManager.drawText(commandBuffer, instances);
    }
}

void RenderEngine::renderTexts(
    VkCommandBuffer commandBuffer, win::Window& window, gfx::mngrs::UiManager& uiManager,
    const std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>>& instancesData) {
    auto& pipeline = m_pipelines["text"];
    pipeline->bind(commandBuffer);
    m_bindlessManager.bind(commandBuffer, pipeline->getPipelineLayout());

    for (const auto& [name, instances] : instancesData) {
        if (instances.empty())
            continue;

        gfx::ui::Text::PushConstantObject push{};
        push.proj = glm::ortho(0.0f, static_cast<float>(window.getWidth()), 0.0f, static_cast<float>(window.getHeight()));
        push.texIndex = uiManager.getTextureIndex(name);

        pipeline->pushConstant(commandBuffer, push);
        uiManager.drawText(commandBuffer, instances);
    }
}

} // namespace game::engines