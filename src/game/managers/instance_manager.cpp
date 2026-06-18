#include "instance_manager.hpp"

namespace game::mngrs {

InstanceManager::InstanceManager() {}

InstanceManager::~InstanceManager() {}

void InstanceManager::rebuildMap(
    const std::shared_ptr<types::Instance>& root, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId, uint32_t nicknameId) {
    for (auto& [name, vec] : m_modelInstancesData) {
        vec.clear();
    }
    m_partDynamicTargets.clear();

    collectInstances(root, studsTexId, smoothTexId, faceTexId, nicknameId);

    m_hierarchyDirty = false;
}

void InstanceManager::collectInstances(
    const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId, uint32_t nicknameId) {
    if (!parent)
        return;

    for (const auto& obj : parent->getChildren()) {
        auto type = obj->getType();
        if (type == enums::InstanceType::Part) {
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

            auto& vec = m_modelInstancesData[bucket];
            vec.push_back(
                {part->getModelMatrix(), glm::vec4(glm::vec3(part->getColor()) / 255.0f, alpha), texture, texTile});

            if (!part->getAnchored()) {
                m_partDynamicTargets.push_back({part, bucket, vec.size() - 1});
            }
        } else if (type == enums::InstanceType::BillboardText) {
            auto billbText = std::static_pointer_cast<types::BillboardText>(obj);

            std::string bucket = "nickname";

            auto& vec = m_billbTextInstancesContent[bucket];
            vec.push_back({billbText->getText(), billbText->getPosition()});

            m_billbTextDynamicTargets.push_back({billbText, bucket, vec.size() - 1});
        }

        collectInstances(obj, studsTexId, smoothTexId, faceTexId, nicknameId);
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
    for (size_t i = 0; i < m_partDynamicTargets.size();) {
        const auto& target = m_partDynamicTargets[i];

        glm::vec3 position = target.part->getPosition();
        if (position.y < -1000.0f) {
            target.part->destroy();

            m_hierarchyDirty = true;

            m_partDynamicTargets.erase(m_partDynamicTargets.begin() + i);

            continue;
        }

        m_modelInstancesData[target.bucketName][target.index].model = target.part->getModelMatrix();
        i++;
    }

    for (const auto& target : m_billbTextDynamicTargets) {
        m_billbTextInstancesContent[target.bucketName][target.index].position = target.billb->getPosition() + target.billb->getOffset();
    }
}

void InstanceManager::sortTransparentInstances(const glm::vec3& cameraPos) {
    if (m_modelInstancesData.contains("cubeTransparent"))
        sortInstances(cameraPos, m_modelInstancesData["cubeTransparent"]);
    if (m_modelInstancesData.contains("sphereTransparent"))
        sortInstances(cameraPos, m_modelInstancesData["sphereTransparent"]);
    if (m_modelInstancesData.contains("headTransparent"))
        sortInstances(cameraPos, m_modelInstancesData["headTransparent"]);
}

} // namespace game::mngrs