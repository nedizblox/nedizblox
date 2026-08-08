#pragma once

#include "instance.hpp"

namespace game::types {

class Game : public Instance {
public:
    Game() : Instance(enums::InstanceType::Game, "game") {}
};

} // namespace game::types