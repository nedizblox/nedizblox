#pragma once

#include "../types/types.hpp"

#include "graphics/model.hpp"

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

    void updateDynamicTransforms();

    void sortTransparentInstances(const glm::vec3& cameraPos);

    const std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>& getInstancesData() const {
        return m_instancesData;
    }

    void markDirty() { m_hierarchyDirty = true; }
    bool isDirty() const { return m_hierarchyDirty; }

private:
    struct DynamicTarget {
        std::shared_ptr<types::Part> part;
        std::string bucketName;
        size_t index;
    };

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_instancesData;

    std::vector<std::shared_ptr<types::Part>> m_dynamicParts;
    std::vector<DynamicTarget> m_dynamicTargets;

    bool m_hierarchyDirty = true;

    void collectInstances(const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId);
    void sortInstances(const glm::vec3& cameraPos, std::vector<gfx::Model::InstanceData>& instances);
};

} // namespace game::mngrs