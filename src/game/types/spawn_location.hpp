#pragma once

#include "part.hpp"
#include "decal.hpp"

namespace game::types {

class SpawnLocation : public Part {
public:
    static std::shared_ptr<SpawnLocation> create(const std::string& name = "SpawnLocation") {
        auto spawn = std::shared_ptr<SpawnLocation>(new SpawnLocation(name));

        auto decal = std::make_shared<Decal>();
        decal->setSource("spawnLocation");
        decal->setParent(spawn);

        return spawn;
    }

private:
    SpawnLocation(const std::string& name = "SpawnLocation") : Part(name) {
        setSize(glm::vec3(5.0f, 1.0f, 5.0f));
        setSurfaceTop(enums::SurfaceType::Smooth);
        setSurfaceBottom(enums::SurfaceType::Smooth);
        setSurfaceLeft(enums::SurfaceType::Glue);
        setSurfaceRight(enums::SurfaceType::Glue);
        setSurfaceFront(enums::SurfaceType::Glue);
        setSurfaceBack(enums::SurfaceType::Glue);
    }
};

} // namespace game::types