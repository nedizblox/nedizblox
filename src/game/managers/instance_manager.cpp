#include "instance_manager.hpp"

#include <algorithm>

namespace game::mngrs {

InstanceManager::InstanceManager(AssetManager& assetManager) : m_assetManager(assetManager) {}

InstanceManager::~InstanceManager() {}

void InstanceManager::rebuildMap(
    const std::shared_ptr<types::Instance>& root, uint32_t studsTexId, uint32_t inletsTexId,
    uint32_t smoothTexId, uint32_t glueTexId) {
    for (auto& [name, vec] : m_modelInstancesData) {
        vec.clear();
    }
    for (auto& [name, vec] : m_billbTextInstancesContent) {
        vec.clear();
    }

    m_partDynamicTargets.clear();
    m_billbTextDynamicTargets.clear();

    collectMapInstances(root, studsTexId, inletsTexId, smoothTexId, glueTexId);

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
    const std::shared_ptr<types::Instance>& parent, uint32_t studsTexId, uint32_t inletsTexId,
    uint32_t smoothTexId, uint32_t glueTexId) {
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

            auto getTex = [&](enums::SurfaceType surface) -> uint32_t {
                switch (surface) {
                case enums::SurfaceType::Studs:
                    return studsTexId;
                case enums::SurfaceType::Inlets:
                    return inletsTexId;
                case enums::SurfaceType::Smooth:
                    return smoothTexId;
                case enums::SurfaceType::Glue:
                    return glueTexId;
                }
            };

            glm::uvec3 textures1{};
            glm::uvec3 textures2{};
            if (shape == enums::PartType::Block) {
                textures1
                    = {getTex(part->getSurfaceLeft()), getTex(part->getSurfaceRight()),
                       getTex(part->getSurfaceTop())};
                textures2
                    = {getTex(part->getSurfaceBottom()), getTex(part->getSurfaceFront()),
                       getTex(part->getSurfaceBack())};
            } else {
                textures1 = {smoothTexId, smoothTexId, smoothTexId};
                textures2 = {smoothTexId, smoothTexId, smoothTexId};
            }

            glm::vec2 texTile{1.0f, 1.0f};
            auto decal = part->findFirstChildOfClass<types::Decal>();
            if (decal) {
                texTile = glm::vec2(1.0f, 0.0f);

                uint32_t texId = 0;
                try {
                    texId = m_assetManager.getTextureId(decal->getSource());
                } catch (const std::exception& e) {}

                switch (decal->getFace()) {
                case enums::Face::Left:
                    textures1.x = texId;
                    break;
                case enums::Face::Right:
                    textures1.y = texId;
                    break;
                case enums::Face::Top:
                    textures1.z = texId;
                    break;
                case enums::Face::Bottom:
                    textures2.x = texId;
                    break;
                case enums::Face::Front:
                    textures2.y = texId;
                    break;
                case enums::Face::Back:
                    textures2.z = texId;
                    break;
                }
            }

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
                {part->getModelMatrix(), glm::vec4(glm::vec3(part->getColor()) / 255.0f, alpha),
                 textures1, textures2, texTile});

            size_t instanceIndex = vec.size() - 1;

            if (!part->getAnchored()) {
                m_partDynamicTargets.push_back({part, bucket, instanceIndex});

                part->onDestroy(
                    [this](std::shared_ptr<types::Instance> destroyed) { markMapDirty(); });
            } else {
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
                });
            }
        } else if (type == enums::InstanceType::BillboardText) {
            auto billbText = std::static_pointer_cast<types::BillboardText>(obj);

            std::string bucket;
            switch (billbText->getTextFont()) {
            case enums::TextFont::Nunito:
                bucket = "nunito";
                break;
            case enums::TextFont::Arial:
                bucket = "arial";
                break;
            }

            auto& vec = m_billbTextInstancesContent[bucket];
            vec.push_back({billbText->getText(), billbText->getPosition(), billbText->getScale()});

            m_billbTextDynamicTargets.push_back({billbText, bucket, vec.size() - 1});
        }

        collectMapInstances(obj, studsTexId, inletsTexId, smoothTexId, glueTexId);
    }
}

void InstanceManager::collectGuiInstances(const std::shared_ptr<types::Instance>& parent) {
    if (!parent)
        return;

    for (const auto& obj : parent->getChildren()) {
        auto type = obj->getType();
        if (type == enums::InstanceType::Text) {
            auto text = std::static_pointer_cast<types::Text>(obj);

            std::string bucket;
            switch (text->getTextFont()) {
            case enums::TextFont::Nunito:
                bucket = "nunito";
                break;
            case enums::TextFont::Arial:
                bucket = "arial";
                break;
            }

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
    for (const auto& target : m_partDynamicTargets) {
        float transparency = target.obj->getTransparency();
        float alpha = (transparency <= 0.0f) ? 1.0f : (1.0f - transparency);

        auto& instances = m_modelInstancesData[target.bucketName][target.index];
        instances.model = target.obj->getModelMatrix();
        instances.color = glm::vec4(glm::vec3(target.obj->getColor()) / 255.0f, alpha);
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