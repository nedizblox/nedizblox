#include "instance_manager.hpp"

#include <algorithm>
#include <glm/gtc/packing.hpp>
#include <numeric>

namespace game::mngrs {

InstanceManager::InstanceManager(AssetManager& assetManager) :
    m_assetManager(assetManager) {}

InstanceManager::~InstanceManager() {}

glm::uvec3 InstanceManager::packTexTiles(const glm::vec2* texTiles) {
    glm::uvec3 packed;
    packed.x = glm::packHalf2x16(glm::vec2(texTiles[0].x, texTiles[1].x));
    packed.y = glm::packHalf2x16(glm::vec2(texTiles[2].x, texTiles[3].x));
    packed.z = glm::packHalf2x16(glm::vec2(texTiles[4].x, texTiles[5].x));
    return packed;
}

std::string InstanceManager::getModelBucketFromShape(enums::PartType shape) {
    switch (shape) {
    case enums::PartType::Block:
        return "cube";
    case enums::PartType::Ball:
        return "sphere";
    case enums::PartType::Cylinder:
        return "cylinder";
    case enums::PartType::Wedge:
        return "wedge";
    case enums::PartType::Head:
        return "head";
    default:
        return "cube";
    }
}

void InstanceManager::rebuildMap(const std::shared_ptr<types::Instance>& root) {
    if (m_currentRoot.lock() != root) {
        if (auto oldRoot = m_currentRoot.lock()) {
            oldRoot->removeDescendantPropertyChangedCallback(m_rootSubscriptionId);
        }

        m_currentRoot = root;
        if (root) {
            m_rootSubscriptionId = root->onDescendantPropertyChanged(
                [this](std::shared_ptr<types::Instance> changed) { this->markMapDirty(); });
        }
    }

    for (auto& [name, vec] : m_opaqueModelInstancesData)
        vec.clear();
    for (auto& [name, vec] : m_transparentModelInstancesData)
        vec.clear();

    for (auto& [name, vec] : m_opaqueModelOutlineInstancesData)
        vec.clear();
    for (auto& [name, vec] : m_transparentModelOutlineInstancesData)
        vec.clear();

    for (auto& [name, vec] : m_billbTextInstancesContent)
        vec.clear();

    m_partDynamicTargets.clear();
    m_selectionBoxDynamicTargets.clear();
    m_billbTextDynamicTargets.clear();

    collectMapInstances(root);

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

void InstanceManager::collectMapInstances(const std::shared_ptr<types::Instance>& parent) {
    if (!parent)
        return;

    for (const auto& obj : parent->getChildren()) {
        auto type = obj->getType();

        if (type == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(obj);

            float transparency = part->getTransparency();
            if (transparency >= 1.0f) {
                collectMapInstances(obj);
                continue;
            }

            bool isTransparent = transparency > 0.0f;

            uint32_t studsId = m_assetManager.getTextureId("studs");
            uint32_t inletsId = m_assetManager.getTextureId("inlets");
            uint32_t glueId = m_assetManager.getTextureId("glue");
            uint32_t smoothId = m_assetManager.getTextureId("smooth");

            uint32_t grassId = m_assetManager.getTextureId("grass");
            uint32_t woodId = m_assetManager.getTextureId("wood");

            enums::MaterialType material = part->getMaterial();
            enums::PartType shape = part->getShape();

            auto getSurfTex = [&](enums::SurfaceType surface) -> uint32_t {
                switch (surface) {
                case enums::SurfaceType::Studs:
                    return studsId;
                case enums::SurfaceType::Inlets:
                    return inletsId;
                case enums::SurfaceType::Smooth:
                    return smoothId;
                case enums::SurfaceType::Glue:
                    return glueId;
                default:
                    return smoothId;
                }
            };

            auto getMatTex = [&](enums::MaterialType material) -> uint32_t {
                switch (material) {
                case enums::MaterialType::SmoothPlastic:
                    return smoothId;
                case enums::MaterialType::Grass:
                    return grassId;
                case enums::MaterialType::Wood:
                    return woodId;
                default:
                    return smoothId;
                }
            };

            glm::uvec3 textures1{};
            glm::uvec3 textures2{};
            if (shape == enums::PartType::Block) {
                if (material == enums::MaterialType::SmoothPlastic) {
                    textures1
                        = {getSurfTex(part->getSurfaceLeft()), getSurfTex(part->getSurfaceRight()),
                           getSurfTex(part->getSurfaceTop())};
                    textures2
                        = {getSurfTex(part->getSurfaceBottom()),
                           getSurfTex(part->getSurfaceFront()), getSurfTex(part->getSurfaceBack())};
                } else {
                    uint32_t matId = getMatTex(material);
                    textures1 = {matId, matId, matId};
                    textures2 = {matId, matId, matId};
                }
            } else {
                textures1 = {smoothId, smoothId, smoothId};
                textures2 = {smoothId, smoothId, smoothId};
            }

            glm::vec2 defaultTile = glm::vec2(1.0f, 0.0f);
            glm::vec2 faceTiles[6]
                = {defaultTile, defaultTile, defaultTile, defaultTile, defaultTile, defaultTile};

            auto decal = part->findFirstChildOfClass<types::Decal>();
            if (decal) {
                uint32_t texId = 0;
                try {
                    texId = m_assetManager.getTextureId(decal->getSource());
                } catch (const std::exception&) {}

                glm::vec2 decalTile{};

                switch (decal->getFace()) {
                case enums::Face::Left:
                    textures1.x = texId;
                    faceTiles[0] = decalTile;
                    break;
                case enums::Face::Right:
                    textures1.y = texId;
                    faceTiles[1] = decalTile;
                    break;
                case enums::Face::Top:
                    textures1.z = texId;
                    faceTiles[2] = decalTile;
                    break;
                case enums::Face::Bottom:
                    textures2.x = texId;
                    faceTiles[3] = decalTile;
                    break;
                case enums::Face::Front:
                    textures2.y = texId;
                    faceTiles[4] = decalTile;
                    break;
                case enums::Face::Back:
                    textures2.z = texId;
                    faceTiles[5] = decalTile;
                    break;
                }
            }

            std::string bucket = getModelBucketFromShape(shape);
            float alpha = isTransparent ? (1.0f - transparency) : 1.0f;

            auto& map = isTransparent ? m_transparentModelInstancesData : m_opaqueModelInstancesData;
            auto& vec = map[bucket];

            vec.push_back(
                {part->getModelMatrix(), glm::vec4(glm::vec3(part->getColor()) / 255.0f, alpha),
                 textures1, textures2, packTexTiles(faceTiles)});

            if (!part->getAnchored()) {
                m_partDynamicTargets.push_back({part, bucket, vec.size() - 1, isTransparent});
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

            auto parent = billbText->getParent();
            if (!parent || parent->getType() != enums::InstanceType::Part)
                continue;

            auto part = std::static_pointer_cast<types::Part>(parent);

            auto& vec = m_billbTextInstancesContent[bucket];
            vec.push_back({billbText->getText(), part->getPosition() + billbText->getOffset(), billbText->getScale()});

            if (!part->getAnchored()) {
                m_billbTextDynamicTargets.push_back({billbText, bucket, vec.size() - 1, true, part});
            }
        } else if (type == enums::InstanceType::SelectionBox) {
            auto selectionBox = std::static_pointer_cast<types::SelectionBox>(obj);

            auto parent = selectionBox->getParent();
            if (!parent || parent->getType() != enums::InstanceType::Part)
                continue;

            auto part = std::static_pointer_cast<types::Part>(parent);

            float transparency = selectionBox->getTransparency();
            if (transparency >= 1.0f)
                continue;

            bool isTransparent = transparency > 0.0f;
            enums::PartType shape = part->getShape();
            std::string bucket = getModelBucketFromShape(shape);

            float alpha = isTransparent ? (1.0f - transparency) : 1.0f;

            auto& map = isTransparent ? m_transparentModelOutlineInstancesData : m_opaqueModelOutlineInstancesData;
            auto& vec = map[bucket];

            vec.push_back({part->getModelMatrix(), glm::vec4(glm::vec3(selectionBox->getColor()) / 255.0f, alpha)});

            if (!part->getAnchored()) {
                m_selectionBoxDynamicTargets.push_back({selectionBox, bucket, vec.size() - 1, isTransparent, part});
            }
        }

        collectMapInstances(obj);
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

            m_textDynamicTargets.push_back({text, bucket, vec.size() - 1, false});
        }

        collectGuiInstances(obj);
    }
}

void InstanceManager::sortInstances(
    const glm::vec3& cameraPos, std::vector<gfx::Model::InstanceData>& instances,
    std::vector<DynamicTarget<types::Part>>& targets, const std::string& bucketName) {
    if (instances.empty())
        return;

    std::vector<size_t> p(instances.size());
    std::iota(p.begin(), p.end(), 0);

    std::sort(p.begin(), p.end(), [&instances, &cameraPos](size_t i1, size_t i2) {
        glm::vec3 posA = glm::vec3(instances[i1].model[3]);
        glm::vec3 posB = glm::vec3(instances[i2].model[3]);

        glm::vec3 diffA = posA - cameraPos;
        glm::vec3 diffB = posB - cameraPos;

        return glm::dot(diffA, diffA) > glm::dot(diffB, diffB);
    });

    std::vector<gfx::Model::InstanceData> sortedInstances(instances.size());
    std::vector<size_t> oldToNewIndex(instances.size());

    for (size_t newIdx = 0; newIdx < p.size(); newIdx++) {
        size_t oldIdx = p[newIdx];
        sortedInstances[newIdx] = instances[oldIdx];
        oldToNewIndex[oldIdx] = newIdx;
    }

    instances = std::move(sortedInstances);

    for (auto& target : targets) {
        if (target.isTransparent && target.bucketName == bucketName) {
            target.index = oldToNewIndex[target.index];
        }
    }
}

void InstanceManager::sortInstances(
    const glm::vec3& cameraPos, std::vector<gfx::ModelOutline::InstanceData>& instances,
    std::vector<DynamicTarget<types::SelectionBox>>& targets, const std::string& bucketName) {
    if (instances.empty())
        return;

    std::vector<size_t> p(instances.size());
    std::iota(p.begin(), p.end(), 0);

    std::sort(p.begin(), p.end(), [&instances, &cameraPos](size_t i1, size_t i2) {
        glm::vec3 posA = glm::vec3(instances[i1].model[3]);
        glm::vec3 posB = glm::vec3(instances[i2].model[3]);

        glm::vec3 diffA = posA - cameraPos;
        glm::vec3 diffB = posB - cameraPos;

        return glm::dot(diffA, diffA) > glm::dot(diffB, diffB);
    });

    std::vector<gfx::ModelOutline::InstanceData> sortedInstances(instances.size());
    std::vector<size_t> oldToNewIndex(instances.size());

    for (size_t newIdx = 0; newIdx < p.size(); newIdx++) {
        size_t oldIdx = p[newIdx];
        sortedInstances[newIdx] = instances[oldIdx];
        oldToNewIndex[oldIdx] = newIdx;
    }

    instances = std::move(sortedInstances);

    for (auto& target : targets) {
        if (target.isTransparent && target.bucketName == bucketName) {
            target.index = oldToNewIndex[target.index];
        }
    }
}

void InstanceManager::updateDynamicTransforms() {
    for (const auto& target : m_partDynamicTargets) {
        float transparency = target.obj->getTransparency();
        float alpha = (transparency <= 0.0f) ? 1.0f : (1.0f - transparency);

        auto& map = target.isTransparent ? m_transparentModelInstancesData : m_opaqueModelInstancesData;
        auto& instance = map[target.bucketName][target.index];

        instance.model = target.obj->getModelMatrix();
        instance.color = glm::vec4(glm::vec3(target.obj->getColor()) / 255.0f, alpha);
    }

    for (const auto& target : m_selectionBoxDynamicTargets) {
        float transparency = target.obj->getTransparency();
        float alpha = (transparency <= 0.0f) ? 1.0f : (1.0f - transparency);

        auto& map = target.isTransparent ? m_transparentModelOutlineInstancesData : m_opaqueModelOutlineInstancesData;
        auto& instance = map[target.bucketName][target.index];

        instance.model = target.parentPart->getModelMatrix();
        instance.outlineColor = glm::vec4(glm::vec3(target.obj->getColor()) / 255.0f, alpha);
    }

    for (const auto& target : m_billbTextDynamicTargets) {
        auto& content = m_billbTextInstancesContent[target.bucketName][target.index];
        content.position = target.parentPart->getPosition() + target.obj->getOffset();
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
    for (auto& [bucketName, instanceVec] : m_transparentModelInstancesData) {
        sortInstances(cameraPos, instanceVec, m_partDynamicTargets, bucketName);
    }

    for (auto& [bucketName, instanceVec] : m_transparentModelOutlineInstancesData) {
        sortInstances(cameraPos, instanceVec, m_selectionBoxDynamicTargets, bucketName);
    }
}

} // namespace game::mngrs