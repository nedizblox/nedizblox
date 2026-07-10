#include "instance_manager.hpp"

#include <algorithm>

namespace game::mngrs {

InstanceManager::InstanceManager() {}

InstanceManager::~InstanceManager() {}

void InstanceManager::rebuildMap(
    const std::shared_ptr<types::Instance>& root, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId) {
    for (auto& [name, vec] : m_modelInstancesData) {
        vec.clear();
    }
    for (auto& [name, vec] : m_billbTextInstancesContent) {
        vec.clear();
    }

    m_partDynamicTargets.clear();
    m_billbTextDynamicTargets.clear();

    collectMapInstances(root, studsTexId, smoothTexId, faceTexId);

    m_mapHierarchyDirty = false;
}

void InstanceManager::rebuildGui(const std::shared_ptr<types::Instance>& root) {
    for (auto& [name, vec] : m_textInstancesContent) {
        vec.clear();
    }

    m_textDynamicTargets.clear();

    collectGuiInstances(root);

    m_guiHierarchyDirty = false;
}

void InstanceManager::collectMapInstances(
    const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t smoothTexId, uint32_t faceTexId) {
    if (!parent)
        return;

    for (const auto& obj : parent->getChildren()) {
        auto type = obj->getType();
        if (type == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(obj);

            float transparency = part->getTransparency();
            if (transparency == 1.0f)
                continue;

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
                    bucket = "headOpaque";
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

            size_t instanceIndex = vec.size() - 1;

            if (!part->getAnchored()) {
                m_partDynamicTargets.push_back({part, bucket, instanceIndex});
            }

            part->onPropertyChanged([this, bucket, instanceIndex](std::shared_ptr<types::Instance> updated) {
                auto part = std::static_pointer_cast<types::Part>(updated);

                if (part->getAnchored() && m_modelInstancesData.contains(bucket)
                    && instanceIndex < m_modelInstancesData[bucket].size()) {
                    float transparency = part->getTransparency();
                    float alpha = (transparency <= 0.0f) ? 1.0f : (1.0f - transparency);

                    auto& renderData = m_modelInstancesData[bucket][instanceIndex];
                    renderData.model = part->getModelMatrix();
                    renderData.color = glm::vec4(glm::vec3(part->getColor()) / 255.0f, alpha);
                }

                if (part->getAnchored()) {
                    auto it = std::find_if(
                        m_partDynamicTargets.begin(), m_partDynamicTargets.end(),
                        [&part](const auto& target) { return target.obj == part; });

                    if (it != m_partDynamicTargets.end()) {
                        m_partDynamicTargets.erase(it);
                    }

                    if (m_modelInstancesData.contains(bucket)
                        && instanceIndex < m_modelInstancesData[bucket].size()) {
                        m_modelInstancesData[bucket][instanceIndex].model = part->getModelMatrix();
                    }
                } else {
                    auto it = std::find_if(
                        m_partDynamicTargets.begin(), m_partDynamicTargets.end(),
                        [&part](const auto& target) { return target.obj == part; });

                    if (it == m_partDynamicTargets.end()) {
                        m_partDynamicTargets.push_back({part, bucket, instanceIndex});
                    }
                }
            });
        } else if (type == enums::InstanceType::BillboardText) {
            auto billbText = std::static_pointer_cast<types::BillboardText>(obj);

            std::string bucket = "nunito";

            auto& vec = m_billbTextInstancesContent[bucket];
            vec.push_back({billbText->getText(), billbText->getPosition(), billbText->getScale()});

            m_billbTextDynamicTargets.push_back({billbText, bucket, vec.size() - 1});
        }

        collectMapInstances(obj, studsTexId, smoothTexId, faceTexId);
    }
}

void InstanceManager::collectGuiInstances(const std::shared_ptr<types::Instance>& parent) {
    if (!parent)
        return;

    for (const auto& obj : parent->getChildren()) {
        auto type = obj->getType();
        if (type == enums::InstanceType::Text) {
            auto text = std::static_pointer_cast<types::Text>(obj);

            std::string bucket = "nunito";

            auto& vec = m_textInstancesContent[bucket];
            vec.push_back({text->getText(), text->getPosition(), text->getScale()});

            m_textDynamicTargets.push_back({text, bucket, vec.size() - 1});
        }

        collectGuiInstances(obj);
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

        glm::vec3 position = target.obj->getPosition();
        if (position.y < -1000.0f) {
            target.obj->destroy();

            m_mapHierarchyDirty = true;

            m_partDynamicTargets.erase(m_partDynamicTargets.begin() + i);

            continue;
        }

        float transparency = target.obj->getTransparency();
        float alpha = (transparency <= 0.0f) ? 1.0f : (1.0f - transparency);

        auto& instances = m_modelInstancesData[target.bucketName][target.index];
        instances.model = target.obj->getModelMatrix();
        instances.color = glm::vec4(glm::vec3(target.obj->getColor()) / 255.0f, alpha);

        i++;
    }

    for (const auto& target : m_billbTextDynamicTargets) {
        auto& content = m_billbTextInstancesContent[target.bucketName][target.index];
        content.position = target.obj->getPosition() + target.obj->getOffset();
        content.scale = target.obj->getScale();
        content.text = target.obj->getText();
    }

    for (const auto& target : m_textDynamicTargets) {
        auto& content = m_textInstancesContent[target.bucketName][target.index];
        content.position = target.obj->getPosition();
        content.scale = target.obj->getScale();
        content.text = target.obj->getText();
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