#include "model_manager.hpp"

namespace gfx::mngrs {

ModelManager::ModelManager(vk::Device& device) : m_device(device) {}

ModelManager::~ModelManager() {}

void ModelManager::loadModel(const std::string& name, const std::string& filePath) {
    if (m_models.contains(name))
        return;

    Managed<Model> managed;
    managed.opaque = Model::createModelFromFile(m_device, filePath);
    managed.transparent = Model::createModelFromFile(m_device, filePath);

    m_models[name] = std::move(managed);
}

void ModelManager::loadModel(const std::string& name, const std::span<const uint8_t>& geometryRaw) {
    if (m_models.contains(name))
        return;

    Managed<Model> managed;
    managed.opaque = Model::createModelFromGeometryRaw(m_device, geometryRaw);
    managed.transparent = Model::createModelFromGeometryRaw(m_device, geometryRaw);

    m_models[name] = std::move(managed);
}

void ModelManager::loadModelOutline(const std::string& name, const std::string& filePath) {
    if (m_modelOutlines.contains(name))
        return;

    Managed<ModelOutline> managed;
    managed.opaque = ModelOutline::createModelFromFile(m_device, filePath);
    managed.transparent = ModelOutline::createModelFromFile(m_device, filePath);

    m_modelOutlines[name] = std::move(managed);
}

void ModelManager::loadModelOutline(const std::string& name, const std::span<const uint8_t>& geometryRaw) {
    if (m_modelOutlines.contains(name))
        return;

    Managed<ModelOutline> managed;
    managed.opaque = ModelOutline::createModelFromGeometryRaw(m_device, geometryRaw);
    managed.transparent = ModelOutline::createModelFromGeometryRaw(m_device, geometryRaw);

    m_modelOutlines[name] = std::move(managed);
}

void ModelManager::loadSkybox() {
    if (m_skybox)
        return;

    m_skybox = std::make_unique<gfx::Skybox>(m_device);
}

void ModelManager::drawOpaque(
    VkCommandBuffer commandBuffer,
    const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData) {
    for (auto& [name, managed] : m_models) {
        std::string bucketName = name + "_opaque";
        auto it = instancesData.find(bucketName);

        if (it == instancesData.end()) {
            it = instancesData.find(name);
        }

        if (it != instancesData.end() && !it->second.empty()) {
            managed.opaque->draw(commandBuffer, it->second);
        }
    }
}

void ModelManager::drawTransparent(
    VkCommandBuffer commandBuffer,
    const std::unordered_map<std::string, std::vector<Model::InstanceData>>& instancesData) {
    for (auto& [name, managed] : m_models) {
        std::string bucketName = name + "_transparent";
        auto it = instancesData.find(bucketName);

        if (it == instancesData.end()) {
            it = instancesData.find(name);
        }

        if (it != instancesData.end() && !it->second.empty()) {
            managed.transparent->draw(commandBuffer, it->second);
        }
    }
}

void ModelManager::drawOutlinesOpaque(
    VkCommandBuffer commandBuffer,
    const std::unordered_map<std::string, std::vector<ModelOutline::InstanceData>>& instancesData) {
    for (auto& [name, managed] : m_modelOutlines) {
        std::string bucketName = name + "_opaque";
        auto it = instancesData.find(bucketName);

        if (it == instancesData.end()) {
            it = instancesData.find(name);
        }

        if (it != instancesData.end() && !it->second.empty()) {
            managed.transparent->draw(commandBuffer, it->second);
        }
    }
}

void ModelManager::drawOutlinesTransparent(
    VkCommandBuffer commandBuffer,
    const std::unordered_map<std::string, std::vector<ModelOutline::InstanceData>>& instancesData) {
    for (auto& [name, managed] : m_modelOutlines) {
        std::string bucketName = name + "_transparent";
        auto it = instancesData.find(bucketName);

        if (it == instancesData.end()) {
            it = instancesData.find(name);
        }

        if (it != instancesData.end() && !it->second.empty()) {
            managed.transparent->draw(commandBuffer, it->second);
        }
    }
}

void ModelManager::drawSkybox(VkCommandBuffer commandBuffer) {
    m_skybox->draw(commandBuffer);
}

} // namespace gfx::mngrs