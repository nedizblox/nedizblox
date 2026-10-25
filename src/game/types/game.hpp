#pragma once

#include "instance.hpp"

namespace game::types {

class Game : public TypedInstance<Game> {
public:
    Game() : TypedInstance<Game>(enums::InstanceType::Game, "game") {}

    void destroy() override {
        return;
    }
};

} // namespace game::types