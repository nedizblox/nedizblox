#pragma once

#include "../model.hpp"
#include "../model_outline.hpp"

#include "../skybox.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace gfx::mngrs {

class ModelManager {
public:
    ModelManager(vk::Device& device);
    ~ModelManager();

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void loadModel(const std::string& name, const std::string& filePath);
    void loadModel(const std::string& name, const std::span<const uint8_t>& geometryRaw);

    void loadModelOutline(const std::string& name, const std::string& filePath);
    void loadModelOutline(const std::string& name, const std::span<const uint8_t>& geometryRaw);

    void loadSkybox();

    void drawOpaque(
        VkCommandBuffer commandBuffer,
        const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData);
    void drawTransparent(
        VkCommandBuffer commandBuffer,
        const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData);

    void drawOutlinesOpaque(
        VkCommandBuffer commandBuffer,
        const std::unordered_map<std::string, std::vector<ModelOutline::InstanceData>>& instancesData);
    void drawOutlinesTransparent(
        VkCommandBuffer commandBuffer,
        const std::unordered_map<std::string, std::vector<ModelOutline::InstanceData>>& instancesData);
    
    void drawSkybox(VkCommandBuffer commandBuffer);

private:
    template <typename T>
    struct Managed {
        std::unique_ptr<T> opaque;
        std::unique_ptr<T> transparent;
    };

    vk::Device& m_device;

    std::unordered_map<std::string, Managed<Model>> m_models;
    std::unordered_map<std::string, Managed<ModelOutline>> m_modelOutlines;

    std::unique_ptr<Skybox> m_skybox;
};

} // namespace gfx::mngrs