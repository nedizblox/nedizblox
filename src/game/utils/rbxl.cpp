#include "rbxl.hpp"

extern "C" {

enum class RbxlClassType { Folder, Model, Part, SpawnLocation };
enum class RbxlPartType { Ball, Block, Cylinder, Wedge };

struct RbxlInstanceData {
    int64_t id;
    int64_t parent_id;
    const char* name;
    RbxlClassType class_type;
};

struct RbxlPartData {
    RbxlInstanceData base;
    float position[3];
    float size[3];
    float orientation[3];
    uint8_t color[3];
    float transparency;
    bool anchored;
    RbxlPartType shape;
};

struct RbxlSpawnLocationData {
    RbxlPartData part;
};

struct RbxlFolderData {
    RbxlInstanceData base;
};

struct RbxlModelData {
    RbxlInstanceData base;
};

union RbxlObjectUnion {
    RbxlInstanceData base;
    RbxlFolderData folder;
    RbxlModelData model;
    RbxlPartData part;
    RbxlSpawnLocationData spawn;
};

struct RbxlObjectData {
    RbxlObjectUnion data;
};

RbxlObjectData* rbxlLoad(const char* path, size_t* out_count);
void rbxlFree(RbxlObjectData* ptr, size_t count);
}

namespace game::utils {

void fillPart(std::shared_ptr<types::Part>& target, const RbxlPartData& src) {
    target->setPosition({src.position[0], src.position[1], src.position[2]});
    target->setOrientation(glm::quat(glm::vec3(src.orientation[0], src.orientation[1], src.orientation[2])));
    target->setSize({src.size[0], src.size[1], src.size[2]});
    target->setColor({src.color[0], src.color[1], src.color[2]});
    target->setTransparency(src.transparency);
    target->setAnchored(src.anchored);
    target->setShape(static_cast<enums::PartShape>(src.shape));
}

std::vector<std::shared_ptr<types::Instance>> rbxl::parseRbxl(const std::string& filePath) {
    size_t count = 0;
    RbxlObjectData* raw = rbxlLoad(filePath.c_str(), &count);
    if (!raw) {
        throw std::runtime_error("RBXL: Failed to load file " + filePath);
    }

    std::unordered_map<int64_t, std::shared_ptr<types::Instance>> instanceMap;
    std::vector<std::shared_ptr<types::Instance>> allInstances;
    allInstances.reserve(count);

    for (size_t i = 0; i < count; i++) {
        const auto& obj = raw[i].data;
        std::shared_ptr<types::Instance> instance;

        switch (obj.base.class_type) {
        case RbxlClassType::Folder: {
            instance = types::Folder::create(obj.folder.base.name ? obj.folder.base.name : "Folder");
            break;
        }
        case RbxlClassType::Model: {
            instance = types::Model::create(obj.model.base.name ? obj.model.base.name : "Model");
            break;
        }
        case RbxlClassType::Part: {
            auto part = types::Part::create(obj.part.base.name ? obj.part.base.name : "Part");
            fillPart(part, obj.part);
            instance = part;
            break;
        }
        case RbxlClassType::SpawnLocation: {
            bool isSpawn = (obj.base.class_type == RbxlClassType::SpawnLocation);

            std::shared_ptr<types::Part> part;
            if (isSpawn) {
                part = types::SpawnLocation::create(obj.base.name ? obj.base.name : "SpawnLocation");
            } else {
                part = types::Part::create(obj.base.name ? obj.base.name : "Part");
            }

            fillPart(part, obj.part);

            instance = part;
            break;
        }
        }

        if (instance) {
            instance->setId(static_cast<uint32_t>(obj.base.id));
        }

        instanceMap[obj.base.id] = instance;
        allInstances.push_back(instance);
    }

    for (size_t i = 0; i < count; i++) {
        const auto& obj = raw[i].data;
        if (obj.base.parent_id != -1) {
            auto childIt = instanceMap.find(obj.base.id);
            auto parentIt = instanceMap.find(obj.base.parent_id);

            if (childIt != instanceMap.end() && parentIt != instanceMap.end()) {
                childIt->second->setParent(parentIt->second);
            }
        }
    }

    rbxlFree(raw, count);
    return allInstances;
}

} // namespace game::utils