#pragma once

#include "types.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace game::types::factory {

using CreatorFunc = std::shared_ptr<Instance> (*)(const std::string&);

struct InstanceTypeOption {
    enums::InstanceType type;
    const char* name;
    CreatorFunc create;
};

template<typename T>
std::shared_ptr<Instance> createInstance(const std::string& name) {
    return T::create(name);
}

inline constexpr InstanceTypeOption kCreatableTypes[] = {
    {enums::InstanceType::Model, "Model", &createInstance<Model>},
    {enums::InstanceType::Folder, "Folder", &createInstance<Folder>},
    {enums::InstanceType::Part, "Part", &createInstance<Part>},
    {enums::InstanceType::SelectionBox, "SelectionBox",
     &createInstance<SelectionBox>},
     {enums::InstanceType::Model, "Rig", &createInstance<Rig>},
    {enums::InstanceType::Part, "SpawnLocation", &createInstance<SpawnLocation>},
    {enums::InstanceType::Humanoid, "Humanoid", &createInstance<Humanoid>},
    {enums::InstanceType::Decal, "Decal", &createInstance<Decal>},
    {enums::InstanceType::BillboardText, "BillboardText",
     &createInstance<BillboardText>},
    {enums::InstanceType::Text, "Text", &createInstance<Text>},
};

inline constexpr size_t kCreatableTypesCount =
    sizeof(kCreatableTypes) / sizeof(kCreatableTypes[0]);

inline const InstanceTypeOption* find(enums::InstanceType type) {
    for (const auto& option : kCreatableTypes) {
        if (option.type == type)
            return &option;
    }
    return nullptr;
}

inline bool canCreate(enums::InstanceType type) {
    return find(type) != nullptr;
}

inline std::shared_ptr<Instance>
create(enums::InstanceType type, const std::string& name = "") {
    const auto* option = find(type);
    if (!option)
        return nullptr;
    return option->create(name.empty() ? std::string(option->name) : name);
}

} // namespace game::types::factory