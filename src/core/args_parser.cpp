#include "args_parser.hpp"

#include <vector>

namespace core {

std::unordered_map<std::string, std::string> argsparser::parseArguments(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> arguments;

    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].rfind("-", 0) == 0) {
            std::string key = args[i];

            key.erase(0, key.find_first_not_of('-'));

            if (i + 1 < args.size() && args[i + 1].rfind("-", 0) != 0) {
                arguments[key] = args[i + 1];
                i++;
            } else {
                arguments[key] = "true";
            }
        }
    }

    return arguments;
}

} // namespace core