#include "game/game.hpp"

int main() {
    try {
        game::Game gameMain;
        gameMain.run();
    } catch (std::exception& e) {
        core::logger::err(e.what());
        return 1;
    }

    return 0;
}