#pragma once

#include "instance.hpp"

namespace game::types {

class Folder : public Instance {
public:
    Folder(const std::string& name) : Instance(enums::InstanceType::Folder, name) {}

    static std::shared_ptr<Folder> create(const std::string& name = "Folder") {
        return std::make_shared<Folder>(name);
    }

    void serialize(net::serializer::BinaryWriter& writer) const override {
        Instance::serialize(writer);
    }

    void deserialize(net::serializer::BinaryReader& reader) override {
        Instance::deserialize(reader);
    }
};

} // namespace game::types