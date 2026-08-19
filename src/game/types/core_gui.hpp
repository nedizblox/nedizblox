#pragma once

#include "instance.hpp"

namespace game::types {

class CoreGui : public TypedInstance<CoreGui> {
public:
    CoreGui() : TypedInstance<CoreGui>(enums::InstanceType::CoreGui, "CoreGui") {}
};

} // namespace game::types