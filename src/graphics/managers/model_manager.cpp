#include "model_manager.hpp"

namespace gfx::mngrs {

ModelManager::ModelManager(vk::Device& device) : m_device(device) {}

ModelManager::~ModelManager() {}

void ModelManager::loadModel(const std::string& name, const std::string& filePath) {
    if (m_models.contains(name))
        return;

    ManagedModel managed;
    managed.opaque = Model::createModelFromFile(m_device, filePath);
    managed.transparent = Model::createModelFromFile(m_device, filePath);

    m_models[name] = std::move(managed);
}

void ModelManager::loadSkybox() {
    if (m_skybox)
        return;

    m_skybox = std::make_unique<gfx::Skybox>(m_device);
}

void ModelManager::drawOpaque(
    VkCommandBuffer commandBuffer,
    const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData) {
    for (auto& [name, model] : m_models) {
        std::string bucketName = name + "Opaque";
        auto it = instancesData.find(bucketName);

        if (it == instancesData.end()) {
            it = instancesData.find(name);
        }

        if (it != instancesData.end() && !it->second.empty()) {
            model.opaque->draw(commandBuffer, it->second);
        }
    }
}

void ModelManager::drawTransparent(
    VkCommandBuffer commandBuffer,
    const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData) {
    for (auto& [name, model] : m_models) {
        std::string bucketName = name + "Transparent";
        auto it = instancesData.find(bucketName);

        if (it == instancesData.end()) {
            it = instancesData.find(name);
        }

        if (it != instancesData.end() && !it->second.empty()) {
            model.transparent->draw(commandBuffer, it->second);
        }
    }
}

void ModelManager::drawSkybox(VkCommandBuffer commandBuffer) {
    m_skybox->draw(commandBuffer);
}

} // namespace gfx::mngrs