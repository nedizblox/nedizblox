#pragma once

#include "core/camera.hpp"
#include "graphics/graphics.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace game::engines {

class RenderEngine {
public:
    RenderEngine(gfx::vk::Device& device, gfx::vk::Renderer& renderer, gfx::mngrs::BindlessManager& bindlessManager);
    ~RenderEngine();

    RenderEngine(const RenderEngine&) = delete;
    RenderEngine& operator=(const RenderEngine&) = delete;

    void initPipelines(VkDescriptorSetLayout setLayout);

    void renderModelsOpaque(
        VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
        gfx::mngrs::ModelManager& modelManager,
        const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData);

    void renderSkybox(
        VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
        gfx::Skybox& skybox, uint32_t skyboxCubemapId);

    void renderModelsTransparent(
        VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
        gfx::mngrs::ModelManager& modelManager,
        const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData);

    void renderBillboardTexts(
        VkCommandBuffer commandBuffer, const core::camera::SphericalCamera& camera,
        gfx::mngrs::BillboardManager& billboardManager,
        const std::unordered_map<std::string, std::vector<gfx::billb::Text::InstanceContent>>& instancesData);

    void renderDebugUI(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, gfx::ui::Text& fpsFont, float deltaTime);

private:
    gfx::vk::Device& m_device;
    gfx::vk::Renderer& m_renderer;
    gfx::mngrs::BindlessManager& m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Pipeline>> m_pipelines;
};

} // namespace game::engines