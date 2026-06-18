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

    void rebuildMap(const std::shared_ptr<types::Instance>& root, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId, uint32_t nicknameId);

    void updateDynamicTransforms();

    void sortTransparentInstances(const glm::vec3& cameraPos);

    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& getModelInstancesData() const {
        return m_modelInstancesData;
    }

    const std::unordered_map<std::string, std::vector<gfx::Billboard::InstanceContent>>& getBillbTextInstancesContent() const {
        return m_billbTextInstancesContent;
    }

    void markDirty() { m_hierarchyDirty = true; }
    bool isDirty() const { return m_hierarchyDirty; }

private:
    struct PartDynamicTarget {
        std::shared_ptr<types::Part> part;
        std::string bucketName;
        size_t index;
    };

    struct BillbTextDynamicTarget {
        std::shared_ptr<types::BillboardText> billb;
        std::string bucketName;
        size_t index;
    };

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_modelInstancesData;
    std::unordered_map<std::string, std::vector<gfx::Billboard::InstanceContent>> m_billbTextInstancesContent;

    std::vector<PartDynamicTarget> m_partDynamicTargets;
    std::vector<BillbTextDynamicTarget> m_billbTextDynamicTargets;

    bool m_hierarchyDirty = true;

    void collectInstances(const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId, uint32_t nicknameId);
    void sortInstances(const glm::vec3& cameraPos, std::vector<gfx::Model::InstanceData>& instances);
};

} // namespace game::mngrs