#include "game/game_server.hpp"

#include "core/core.hpp"

#include <charconv>
#include <stdexcept>

int main(int argc, char* argv[]) {
    core::logger::init();

    auto args = core::argsparser::parseArguments(argc, argv);

    try {
        std::unique_ptr<game::GameServer> game;

        if (args.contains("port") && args.contains("map")) {
            std::string portStr = args["port"];
            std::string map = args["map"];
            uint16_t port = 0;

            auto [ptr, ec] = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);

            if (ec == std::errc::result_out_of_range) {
                throw std::runtime_error("Port is too big or invalid, maximum is 65535");
            } else if (ec != std::errc()) {
                throw std::runtime_error("Port must be a valid number");
            }

            game = std::make_unique<game::GameServer>(port, map);
        } else {
            throw std::runtime_error("Please specify the server port and rbxl map path using the --port and --map argument");
        }

        game->run();
    } catch (const std::exception& e) {
        core::logger::err(e.what());
        return 1;
    }

    return 0;
}