#pragma once

#include "../vk/device.hpp"
#include "../vk/pipeline.hpp"
#include "../vk/renderer.hpp"

#include "../shadow_map.hpp"

#include "billboard_manager.hpp"
#include "bindless_manager.hpp"
#include "model_manager.hpp"
#include "ui_manager.hpp"

#include <string>
#include <unordered_map>

namespace gfx::mngrs {

class PipelineManager {
public:
    PipelineManager(
        gfx::vk::Device& device, gfx::vk::Renderer& renderer,
        gfx::mngrs::BindlessManager& bindlessManager,
        gfx::mngrs::ModelManager& modelManager,
        gfx::mngrs::BillboardManager& billboardManager,
        gfx::mngrs::UiManager& uiManager
    );
    ~PipelineManager();

    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;

    void initPipelines(VkDescriptorSetLayout setLayout);

    void renderShadowPass(
        VkCommandBuffer commandBuffer, const glm::mat4& lightSpace,
        float orthoRadius,
        const std::unordered_map<
            std::string, std::vector<gfx::Model::InstanceData>>& instancesData,
        const gfx::ShadowMap::Settings& settings = {}
    );

    uint32_t getShadowMapSize() const {
        return m_shadowMap->getSize();
    }

    void renderModelsOpaque(
        VkCommandBuffer commandBuffer, const glm::mat4& proj,
        const glm::mat4& view, const glm::vec3& pos,
        const std::unordered_map<
            std::string, std::vector<gfx::Model::InstanceData>>& instancesData
    );

    void renderModelOutlinesOpaque(
        VkCommandBuffer commandBuffer, const glm::mat4& proj,
        const glm::mat4& view, const glm::vec3& pos,
        const std::unordered_map<
            std::string, std::vector<gfx::ModelOutline::InstanceData>>&
            instancesData
    );

    void renderSkybox(
        VkCommandBuffer commandBuffer, const glm::mat4& proj,
        const glm::mat4& view, uint32_t skyboxCubId
    );

    void renderModelsTransparent(
        VkCommandBuffer commandBuffer, const glm::mat4& proj,
        const glm::mat4& view, const glm::vec3& pos,
        const std::unordered_map<
            std::string, std::vector<gfx::Model::InstanceData>>& instancesData
    );

    void renderModelOutlinesTransparent(
        VkCommandBuffer commandBuffer, const glm::mat4& proj,
        const glm::mat4& view, const glm::vec3& pos,
        const std::unordered_map<
            std::string, std::vector<gfx::ModelOutline::InstanceData>>&
            instancesData
    );

    void renderBillboardTexts(
        VkCommandBuffer commandBuffer, const glm::mat4& proj,
        const glm::mat4& view,
        const std::unordered_map<
            std::string, std::vector<gfx::billb::Text::InstanceContent>>&
            instancesData
    );

    void renderTexts(
        VkCommandBuffer commandBuffer, win::Window& window,
        const std::unordered_map<
            std::string, std::vector<gfx::ui::Text::InstanceContent>>&
            instancesData
    );

    void renderImgui(VkCommandBuffer commandBuffer, win::Window& window);

private:
    gfx::vk::Device& m_device;
    gfx::vk::Renderer& m_renderer;

    gfx::mngrs::BindlessManager& m_bindlessManager;
    gfx::mngrs::ModelManager& m_modelManager;
    gfx::mngrs::BillboardManager& m_billboardManager;
    gfx::mngrs::UiManager& m_uiManager;

    std::unique_ptr<gfx::ShadowMap> m_shadowMap;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Pipeline>>
        m_pipelines;
};

} // namespace gfx::mngrs