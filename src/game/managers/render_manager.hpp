#pragma once

#include "core/camera.hpp"

#include "graphics/graphics.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace game::mngrs {

class RenderManager {
public:
    RenderManager(
        gfx::vk::Device& device, gfx::vk::Renderer& renderer,
        gfx::mngrs::BindlessManager& bindlessManager, gfx::mngrs::ModelManager& modelManager,
        gfx::mngrs::BillboardManager& billboardManager, gfx::mngrs::UiManager& uiManager);
    ~RenderManager();

    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;

    void initPipelines(VkDescriptorSetLayout setLayout);

    void renderModelsOpaque(
        VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, const glm::vec3& pos,
        const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData);

    void renderSkybox(VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, uint32_t skyboxCubId);

    void renderModelsTransparent(
        VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view, const glm::vec3& pos,
        const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& instancesData);

    void renderBillboardTexts(
        VkCommandBuffer commandBuffer, const glm::mat4& proj, const glm::mat4& view,
        const std::unordered_map<std::string, std::vector<gfx::Billboard::InstanceContent>>& instancesData);

    void renderTexts(
        VkCommandBuffer commandBuffer, win::Window& window,
        const std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>>& instancesData);

    void renderImgui(VkCommandBuffer commandBuffer, win::Window& window, gfx::ui::Imgui& imgui);

private:
    gfx::vk::Device& m_device;
    gfx::vk::Renderer& m_renderer;

    gfx::mngrs::BindlessManager& m_bindlessManager;
    gfx::mngrs::ModelManager& m_modelManager;
    gfx::mngrs::BillboardManager& m_billboardManager;
    gfx::mngrs::UiManager& m_uiManager;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Pipeline>> m_pipelines;
};

} // namespace game::mngrs