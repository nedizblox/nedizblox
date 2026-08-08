#include "game/game_client.hpp"

#include <charconv>
#include <stdexcept>

int main(int argc, char* argv[]) {
    core::logger::init();
    
    auto args = core::argsparser::parseArguments(argc, argv);

    try {
        std::unique_ptr<game::GameClient> game;
        
        if (args.contains("address") && args.contains("port") && args.contains("nickname")) {
            std::string address = args["address"];
            std::string portStr = args["port"];
            std::string nickname = args["nickname"];

            uint16_t port = 0;

            auto [ptr, ec] = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);

            if (ec == std::errc::result_out_of_range) {
                throw std::runtime_error("Port is too big or invalid, maximum is 65535");
            } else if (ec != std::errc()) {
                throw std::runtime_error("Port must be a valid number");
            }

            game = std::make_unique<game::GameClient>(address, port, nickname);
        } else {
            throw std::runtime_error("Please specify the server address, port and your nickname using the --address, --port and --nickname argument");
        }

        game->run();
    } catch (const std::exception& e) {
        core::logger::err(e.what());
        core::msgbox::showError("Nedizblox Client", e.what());
        return 1;
    }

    return 0;
}