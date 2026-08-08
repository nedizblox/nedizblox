#include "rbxl.hpp"

#include <glm/gtc/quaternion.hpp>

#include <cstdlib>
#include <stdexcept>
#include <unordered_map>

extern "C" {

enum RbxlPartType { Ball, Block, Cylinder, Wedge };

struct RbxlPartData {
    int64_t id;
    int64_t parent_id;
    bool is_container;
    const char* name;
    float position[3];
    float size[3];
    float orientation[3];
    uint8_t color[3];
    float transparency;
    bool anchored;
    RbxlPartType shape;
    bool is_spawn_location;
};

RbxlPartData* rbxlLoad(const char* path, size_t* out_count);
void rbxlFree(RbxlPartData* ptr, size_t count);
}

namespace game::utils::rbxl {

std::vector<std::shared_ptr<types::Instance>> parseRbxl(const std::string& filePath) {
    size_t count = 0;
    RbxlPartData* raw = rbxlLoad(filePath.c_str(), &count);
    if (!raw) {
        throw std::runtime_error("RBXL: Failed to load file " + filePath);
    }

    std::unordered_map<int64_t, std::shared_ptr<types::Instance>> instanceMap;

    std::vector<std::shared_ptr<types::Instance>> allInstances;
    allInstances.reserve(count);

    for (size_t i = 0; i < count; i++) {
        const RbxlPartData& p = raw[i];

        std::shared_ptr<types::Instance> instance;

        if (p.is_container) {
            instance = std::make_shared<types::Instance>(enums::InstanceType::Model, p.name ? p.name : "Folder");
        } else {
            std::shared_ptr<types::Part> part;
            if (p.is_spawn_location) {
                part = std::make_shared<types::Part>(p.name ? p.name : "SpawnLocation");
            } else {
                part = std::make_shared<types::Part>(p.name ? p.name : "Part");
            }

            part->setPosition({p.position[0], p.position[1], p.position[2]});
            part->setOrientation(glm::quat(glm::vec3(p.orientation[0], p.orientation[1], p.orientation[2])));
            part->setSize({p.size[0], p.size[1], p.size[2]});

            part->setColor({p.color[0], p.color[1], p.color[2]});
            part->setTransparency(p.transparency);
            part->setAnchored(p.anchored);
            part->setShape(static_cast<enums::PartType>(p.shape));

            instance = part;
        }

        instanceMap[p.id] = instance;
        allInstances.push_back(instance);
    }

    for (size_t i = 0; i < count; i++) {
        const RbxlPartData& p = raw[i];

        if (p.parent_id != -1) {
            auto childIt = instanceMap.find(p.id);
            auto parentIt = instanceMap.find(p.parent_id);

            if (childIt != instanceMap.end() && parentIt != instanceMap.end()) {
                childIt->second->setParent(parentIt->second);
            }
        }
    }

    rbxlFree(raw, count);

    return allInstances;
}

} // namespace game::utils::rbxl