#pragma once

#include "../types/types.hpp"
#include "asset_manager.hpp"
#include "graphics/graphics.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace game::mngrs {

class InstanceManager {
public:
    InstanceManager(AssetManager& assetManager);
    ~InstanceManager();

    InstanceManager(const InstanceManager&) = delete;
    InstanceManager& operator=(const InstanceManager&) = delete;

    void rebuildMap(const std::shared_ptr<types::Instance>& root);
    void rebuildGui(const std::shared_ptr<types::Instance>& root);

    void updateDynamicTransforms();

    void sortTransparentInstances(const glm::vec3& cameraPos);

    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& getOpaqueModelInstancesData() const {
        return m_opaqueModelInstancesData;
    }

    const std::unordered_map<std::string, std::vector<gfx::ModelOutline::InstanceData>>& getOpaqueModelOutlineInstancesData() const {
        return m_opaqueModelOutlineInstancesData;
    }

    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& getTransparentModelInstancesData() const {
        return m_transparentModelInstancesData;
    }

    const std::unordered_map<std::string, std::vector<gfx::ModelOutline::InstanceData>>& getTransparentModelOutlineInstancesData() const {
        return m_transparentModelOutlineInstancesData;
    }

    const std::unordered_map<std::string, std::vector<gfx::billb::Text::InstanceContent>>&
    getBillbTextInstancesContent() const {
        return m_billbTextInstancesContent;
    }

    const std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>>&
    getTextInstancesContent() const {
        return m_textInstancesContent;
    }

    void markMapDirty() { m_mapHierarchyDirty = true; }
    bool isMapDirty() const { return m_mapHierarchyDirty; }

    void markGuiDirty() { m_guiHierarchyDirty = true; }
    bool isGuiDirty() const { return m_guiHierarchyDirty; }

private:
    AssetManager& m_assetManager;

    template <typename T>
    struct DynamicTarget {
        std::shared_ptr<T> obj;
        std::string bucketName;
        size_t index;
        bool isTransparent;
        std::shared_ptr<types::Part> parentPart = nullptr;
    };

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_opaqueModelInstancesData;
    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_transparentModelInstancesData;

    std::unordered_map<std::string, std::vector<gfx::ModelOutline::InstanceData>> m_opaqueModelOutlineInstancesData;
    std::unordered_map<std::string, std::vector<gfx::ModelOutline::InstanceData>> m_transparentModelOutlineInstancesData;

    std::unordered_map<std::string, std::vector<gfx::billb::Text::InstanceContent>> m_billbTextInstancesContent;
    std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>> m_textInstancesContent;

    std::vector<DynamicTarget<types::Part>> m_partDynamicTargets;
    std::vector<DynamicTarget<types::SelectionBox>> m_selectionBoxDynamicTargets;
    std::vector<DynamicTarget<types::BillboardText>> m_billbTextDynamicTargets;
    std::vector<DynamicTarget<types::Text>> m_textDynamicTargets;

    std::weak_ptr<types::Instance> m_currentRoot;
    types::Instance::CallbackId m_rootSubscriptionId = 0;

    bool m_mapHierarchyDirty = true;
    bool m_guiHierarchyDirty = true;

    glm::uvec3 packTexTiles(const glm::vec2* texTiles);
    std::string getModelBucketFromShape(enums::PartType shape);

    void collectMapInstances(const std::shared_ptr<types::Instance>& parent);
    void collectGuiInstances(const std::shared_ptr<types::Instance>& parent);

    void sortInstances(const glm::vec3& cameraPos, std::vector<gfx::Model::InstanceData>& instances, std::vector<DynamicTarget<types::Part>>& targets, const std::string& bucketName);
    void sortInstances(const glm::vec3& cameraPos, std::vector<gfx::ModelOutline::InstanceData>& instances, std::vector<DynamicTarget<types::SelectionBox>>& targets, const std::string& bucketName);
};

} // namespace game::mngrs