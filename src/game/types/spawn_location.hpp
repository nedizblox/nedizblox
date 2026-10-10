#pragma once

#include "decal.hpp"
#include "part.hpp"

namespace game::types {

class SpawnLocation : public Part {
public:
    SpawnLocation(const std::string& name = "SpawnLocation") :
        Part(name, enums::PartType::SpawnLocation) {
        setSize(glm::vec3(5.0f, 1.0f, 5.0f));
        setSurfaceTop(enums::SurfaceType::Smooth);
        setSurfaceBottom(enums::SurfaceType::Smooth);
        setSurfaceLeft(enums::SurfaceType::Glue);
        setSurfaceRight(enums::SurfaceType::Glue);
        setSurfaceFront(enums::SurfaceType::Glue);
        setSurfaceBack(enums::SurfaceType::Glue);
    }

    static std::shared_ptr<SpawnLocation> create(const std::string& name = "SpawnLocation") {
        auto spawn = std::shared_ptr<SpawnLocation>(new SpawnLocation(name));

        auto decal = std::make_shared<Decal>();
        decal->setSource("spawnLocation");
        decal->setParent(spawn);

        return spawn;
    }

    void serialize(net::serializer::BinaryWriter& writer) const override {
        Part::serialize(writer);
    }

    void deserialize(net::serializer::BinaryReader& reader) override { Part::deserialize(reader); }

private:
};

} // namespace game::types