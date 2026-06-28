#pragma once

#include "instance.hpp"

namespace game::types {

class CoreGui : public Instance {
public:
    CoreGui() : Instance(enums::InstanceType::CoreGui, "CoreGui") {}
};

} // namespace game::types