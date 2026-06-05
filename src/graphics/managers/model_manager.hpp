#pragma once

#include "../model.hpp"

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

    void drawOpaque(
        VkCommandBuffer commandBuffer,
        const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData);
    void drawTransparent(
        VkCommandBuffer commandBuffer,
        const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData);

private:
    struct ManagedModel {
        std::unique_ptr<Model> opaque;
        std::unique_ptr<Model> transparent;
    };

    vk::Device& m_device;

    std::unordered_map<std::string, ManagedModel> m_models;
};

} // namespace gfx::mngrs