#include "game/game_client.hpp"

#include "core/core.hpp"

#include <charconv>
#include <stdexcept>

int main(int argc, char* argv[]) {
    core::logger::init();

    core::logger::info(
        "Nedizblox Client v", PROJECT_VERSION_MAJOR, ".", PROJECT_VERSION_MINOR,
        ".", PROJECT_VERSION_PATCH
    );

    auto args = core::argsparser::parseArguments(argc, argv);

    try {
        std::unique_ptr<game::GameClient> game;

        bool debugTools = false;
        if (args.contains("debug-tools"))
            debugTools = true;

        if (!args.contains("studio")) {
            if (args.contains("address") && args.contains("port") &&
                args.contains("nickname")) {
                std::string address = args["address"];
                std::string portStr = args["port"];
                std::string nickname = args["nickname"];

                uint16_t port = 0;

                auto [ptr, ec] = std::from_chars(
                    portStr.data(), portStr.data() + portStr.size(), port
                );

                if (ec == std::errc::result_out_of_range) {
                    throw std::runtime_error(
                        "Port is too big or invalid, maximum is 65535"
                    );
                } else if (ec != std::errc()) {
                    throw std::runtime_error("Port must be a valid number");
                }

                game =
                    std::make_unique<game::GameClient>(address, port, nickname, debugTools);
            } else {
                throw std::runtime_error(
                    "Please specify the server address, port and your nickname "
                    "using the --address, --port and --nickname argument"
                );
            }
        } else {
            game = std::make_unique<game::GameClient>(debugTools);

            if (args.contains("map"))
                game->buildMap(args["map"]);
        }

        game->run();
    }
    catch (const std::exception& e) {
        core::logger::err(e.what());
        win::Window::spawnMessageBox(
            "Nedizblox Client", e.what(), SDL_MESSAGEBOX_ERROR
        );
        return 1;
    }

    return 0;
}