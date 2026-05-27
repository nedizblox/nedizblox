#include "game/game.hpp"

int main(int argc, char* argv[]) {
    try {
        game::Game gameMain;

        if (argc > 1) {
            gameMain.buildMap(argv[1]);
        } else {
            gameMain.buildMap("assets/maps/Baseplate_test.rbxl");
        }

        gameMain.run();
    } catch (std::exception& e) {
        core::logger::err(e.what());
        return 1;
    }

    return 0;
}