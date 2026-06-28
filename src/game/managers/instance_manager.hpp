#pragma once

#include "../types/types.hpp"

#include "graphics/graphics.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace game::mngrs {

class InstanceManager {
public:
    InstanceManager();
    ~InstanceManager();

    InstanceManager(const InstanceManager&) = delete;
    InstanceManager& operator=(const InstanceManager&) = delete;

    void rebuildMap(const std::shared_ptr<types::Instance>& root, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId);
    void rebuildGui(const std::shared_ptr<types::Instance>& root);

    void updateDynamicTransforms();

    void sortTransparentInstances(const glm::vec3& cameraPos);

    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& getModelInstancesData() const {
        return m_modelInstancesData;
    }

    const std::unordered_map<std::string, std::vector<gfx::Billboard::InstanceContent>>&
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
    template <typename T>
    struct DynamicTarget {
        std::shared_ptr<T> obj;
        std::string bucketName;
        size_t index;
    };

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_modelInstancesData;
    std::unordered_map<std::string, std::vector<gfx::Billboard::InstanceContent>> m_billbTextInstancesContent;
    std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>> m_textInstancesContent;

    std::vector<DynamicTarget<types::Part>> m_partDynamicTargets;
    std::vector<DynamicTarget<types::BillboardText>> m_billbTextDynamicTargets;

    std::vector<DynamicTarget<types::Text>> m_textDynamicTargets;

    bool m_mapHierarchyDirty = true;
    bool m_guiHierarchyDirty = true;

    void collectMapInstances(const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId);
    void collectGuiInstances(const std::shared_ptr<types::Instance>& parent);

    void sortInstances(const glm::vec3& cameraPos, std::vector<gfx::Model::InstanceData>& instances);
};

} // namespace game::mngrs