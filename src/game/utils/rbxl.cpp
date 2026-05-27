#include "rbxl.hpp"

#include <glm/gtc/quaternion.hpp>

#include <cstdlib>

extern "C" {

enum RbxlPartType { Ball, Block, Cylinder, Wedge };

struct RbxlPartData {
    const char* name;
    float position[3];
    float size[3];
    float orientation[3];
    uint8_t color[3];
    float transparency;
    bool anchored;
    RbxlPartType shape;
};

RbxlPartData* rbxlLoad(const char* path, size_t* out_count);
void rbxlFree(RbxlPartData* ptr, size_t count);
}

namespace game::utils {

std::vector<std::shared_ptr<types::Part>> rbxl::parseRbxl(const std::string& filePath) {
    std::vector<std::shared_ptr<types::Part>> out;

    size_t count = 0;
    RbxlPartData* raw = rbxlLoad(filePath.c_str(), &count);
    if (!raw) {
        throw std::runtime_error("RBXL: Failed to open file");
    }

    out.reserve(count);

    for (size_t i = 0; i < count; i++) {
        RbxlPartData& p = raw[i];

        auto part = std::make_shared<types::Part>();

        part->setName(p.name);
        part->setPosition({p.position[0], p.position[1], p.position[2]});
        part->setOrientation(glm::quat(glm::vec3(p.orientation[0], p.orientation[1], p.orientation[2])));
        part->setSize({p.size[0], p.size[1], p.size[2]});

        part->setColor({p.color[0], p.color[1], p.color[2]});
        part->setTransparency(p.transparency);

        part->setAnchored(p.anchored);

        part->setShape(static_cast<enums::PartType>(p.shape));

        out.push_back(std::move(part));
    }

    rbxlFree(raw, count);

    return out;
}

} // namespace game::utils