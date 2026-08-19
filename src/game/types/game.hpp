#pragma once

#include "instance.hpp"

namespace game::types {

class Game : public TypedInstance<Game> {
public:
    Game() : TypedInstance<Game>(enums::InstanceType::Game, "game") {}
};

} // namespace game::types