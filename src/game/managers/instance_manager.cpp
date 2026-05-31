#include "instance_manager.hpp"

namespace game::mngrs {

InstanceManager::InstanceManager() {}

InstanceManager::~InstanceManager() {}

void InstanceManager::rebuildMap(
    const std::shared_ptr<types::Instance>& root, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId) {
    for (auto& [name, vec] : m_instancesData) {
        vec.clear();
    }
    m_dynamicTargets.clear();

    collectInstances(root, studsTexId, smoothTexId, faceTexId);

    m_hierarchyDirty = false;
}

void InstanceManager::collectInstances(
    const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId) {
    if (!parent)
        return;

    for (const auto& obj : parent->getChildren()) {
        if (obj->getType() == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(obj);

            float transparency = part->getTransparency();
            enums::PartType shape = part->getShape();

            uint32_t texture = 0;
            if (shape == enums::PartType::Block) {
                texture = studsTexId;
            } else if (shape == enums::PartType::Ball) {
                texture = smoothTexId;
            } else if (shape == enums::PartType::Head) {
                texture = faceTexId;
            }

            glm::vec2 texTile
                = (shape == enums::PartType::Head) ? glm::vec2(1.0f, 0.0f) : glm::vec2(1.0f, 1.0f);

            std::string bucket = "";
            if (transparency <= 0.0f) {
                switch (shape) {
                case enums::PartType::Block:
                    bucket = "cubeOpaque";
                    break;
                case enums::PartType::Ball:
                    bucket = "sphereOpaque";
                    break;
                case enums::PartType::Head:
                    bucket = "headTransparent";
                    break;
                default:
                    bucket = "cubeOpaque";
                    break;
                }
            } else {
                switch (shape) {
                case enums::PartType::Block:
                    bucket = "cubeTransparent";
                    break;
                case enums::PartType::Ball:
                    bucket = "sphereTransparent";
                    break;
                case enums::PartType::Head:
                    bucket = "headTransparent";
                    break;
                default:
                    bucket = "cubeTransparent";
                    break;
                }
            }

            float alpha = (transparency <= 0.0f) ? 1.0f : (1.0f - transparency);

            auto& vec = m_instancesData[bucket];
            vec.push_back(
                {part->getModelMatrix(), glm::vec4(glm::vec3(part->getColor()) / 255.0f, alpha), texture, texTile});

            if (!part->getAnchored()) {
                m_dynamicTargets.push_back({part, bucket, vec.size() - 1});
            }
        }

        collectInstances(obj, studsTexId, smoothTexId, faceTexId);
    }
}

void InstanceManager::sortInstances(const glm::vec3& cameraPos, std::vector<gfx::Model::InstanceData>& instances) {
    std::sort(instances.begin(), instances.end(), [&cameraPos](const auto& a, const auto& b) {
        glm::vec3 posA = glm::vec3(a.model[3]);
        glm::vec3 posB = glm::vec3(b.model[3]);

        glm::vec3 diffA = posA - cameraPos;
        glm::vec3 diffB = posB - cameraPos;

        return glm::dot(diffA, diffA) > glm::dot(diffB, diffB);
    });
}

void InstanceManager::updateDynamicTransforms() {
    for (const auto& target : m_dynamicTargets) {
        m_instancesData[target.bucketName][target.index].model = target.part->getModelMatrix();
    }
}

void InstanceManager::sortTransparentInstances(const glm::vec3& cameraPos) {
    if (m_instancesData.contains("cubeTransparent"))
        sortInstances(cameraPos, m_instancesData["cubeTransparent"]);
    if (m_instancesData.contains("sphereTransparent"))
        sortInstances(cameraPos, m_instancesData["sphereTransparent"]);
    if (m_instancesData.contains("headTransparent"))
        sortInstances(cameraPos, m_instancesData["headTransparent"]);
}

} // namespace game::mngrs