#pragma once

#include "../types/types.hpp"

#include "graphics/graphics.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace game::mngrs {

class RenderManager {
public:
    RenderManager(gfx::mngrs::TextureManager& textureManager);
    ~RenderManager();

    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;

    size_t getMapRebuildCount() const {
        return m_mapRebuildCount;
    }
    size_t getGuiRebuildCount() const {
        return m_guiRebuildCount;
    }

    void rebuildMap(const std::shared_ptr<types::Instance>& root);
    void rebuildGui(const std::shared_ptr<types::Instance>& root);

    void updateDynamicTransforms();

    void cullInstances(const glm::vec3& cameraPos);

    void setRenderDistance(float distance) {
        m_renderDistance = distance;
    }
    float getRenderDistance() const {
        return m_renderDistance;
    }

    void setMaxInstancesPerBucket(size_t count) {
        m_maxInstancesPerBucket = count;
    }
    size_t getMaxInstancesPerBucket() const {
        return m_maxInstancesPerBucket;
    }

    size_t getTotalInstanceCount() const {
        return m_totalInstanceCount;
    }
    size_t getVisibleInstanceCount() const {
        return m_visibleInstanceCount;
    }

    const std::unordered_map<
        std::string, std::vector<gfx::Model::InstanceData>>&
    getOpaqueModelInstancesData() const {
        return m_visibleOpaqueModels;
    }

    const std::unordered_map<
        std::string, std::vector<gfx::ModelOutline::InstanceData>>&
    getOpaqueModelOutlineInstancesData() const {
        return m_visibleOpaqueOutlines;
    }

    const std::unordered_map<
        std::string, std::vector<gfx::Model::InstanceData>>&
    getTransparentModelInstancesData() const {
        return m_visibleTransparentModels;
    }

    const std::unordered_map<
        std::string, std::vector<gfx::ModelOutline::InstanceData>>&
    getTransparentModelOutlineInstancesData() const {
        return m_visibleTransparentOutlines;
    }

    const std::unordered_map<
        std::string, std::vector<gfx::billb::Text::InstanceContent>>&
    getBillbTextInstancesContent() const {
        return m_billbTextInstancesContent;
    }

    const std::unordered_map<
        std::string, std::vector<gfx::ui::Text::InstanceContent>>&
    getTextInstancesContent() const {
        return m_textInstancesContent;
    }

    void markMapDirty() {
        m_mapHierarchyDirty = true;
    }
    bool isMapDirty() const {
        return m_mapHierarchyDirty;
    }

    void markGuiDirty() {
        m_guiHierarchyDirty = true;
    }
    bool isGuiDirty() const {
        return m_guiHierarchyDirty;
    }

private:
    size_t m_mapRebuildCount = 0;
    size_t m_guiRebuildCount = 0;

    gfx::mngrs::TextureManager& m_textureManager;

    template<typename T>
    struct DynamicTarget {
        std::shared_ptr<T> obj;
        std::string bucketName;
        size_t index;
        bool isTransparent;
        std::shared_ptr<types::Part> parentPart = nullptr;
    };

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>
        m_opaqueModelInstancesData;
    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>
        m_transparentModelInstancesData;

    std::unordered_map<
        std::string, std::vector<gfx::ModelOutline::InstanceData>>
        m_opaqueModelOutlineInstancesData;
    std::unordered_map<
        std::string, std::vector<gfx::ModelOutline::InstanceData>>
        m_transparentModelOutlineInstancesData;

    std::unordered_map<
        std::string, std::vector<gfx::billb::Text::InstanceContent>>
        m_billbTextInstancesContent;
    std::unordered_map<std::string, std::vector<gfx::ui::Text::InstanceContent>>
        m_textInstancesContent;

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>
        m_visibleOpaqueModels;
    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>>
        m_visibleTransparentModels;
    std::unordered_map<
        std::string, std::vector<gfx::ModelOutline::InstanceData>>
        m_visibleOpaqueOutlines;
    std::unordered_map<
        std::string, std::vector<gfx::ModelOutline::InstanceData>>
        m_visibleTransparentOutlines;

    std::vector<std::pair<float, uint32_t>> m_cullScratch;

    float m_renderDistance = 300.0f;
    size_t m_maxInstancesPerBucket = gfx::Model::kMaxInstanceCount;

    size_t m_totalInstanceCount = 0;
    size_t m_visibleInstanceCount = 0;

    std::vector<DynamicTarget<types::Part>> m_partDynamicTargets;
    std::vector<DynamicTarget<types::SelectionBox>>
        m_selectionBoxDynamicTargets;
    std::vector<DynamicTarget<types::BillboardText>> m_billbTextDynamicTargets;
    std::vector<DynamicTarget<types::Text>> m_textDynamicTargets;

    std::weak_ptr<types::Instance> m_currentMapRoot;
    types::Instance::CallbackId m_mapPropSubId = 0;
    types::Instance::CallbackId m_mapChildrenSubId = 0;
    types::Instance::CallbackId m_mapDescendantChildrenSubId = 0;

    std::weak_ptr<types::Instance> m_currentGuiRoot;
    types::Instance::CallbackId m_guiPropSubId = 0;
    types::Instance::CallbackId m_guiChildrenSubId = 0;
    types::Instance::CallbackId m_guiDescendantChildrenSubId = 0;

    bool m_mapHierarchyDirty = true;
    bool m_guiHierarchyDirty = true;

    void collectMapInstances(const std::shared_ptr<types::Instance>& parent);
    void collectGuiInstances(const std::shared_ptr<types::Instance>& parent);

    template<typename T>
    void cullBuckets(
        const std::unordered_map<std::string, std::vector<T>>& source,
        std::unordered_map<std::string, std::vector<T>>& visible,
        const glm::vec3& cameraPos, bool backToFront
    );
};

} // namespace game::mngrs