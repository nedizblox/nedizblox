#include "logger.hpp"

#include <iostream>

#define ANSI_COLOR_BLUE "\x1b[94m"
#define ANSI_COLOR_YELLOW "\x1b[93m"
#define ANSI_COLOR_RED "\x1b[91m"
#define ANSI_COLOR_GREEN "\x1b[92m"

#ifndef NDEBUG
#define ANSI_COLOR_PINK "\x1b[95m"
#endif

#define ANSI_COLOR_RESET "\x1b[39m"

namespace core {

void logger::info(const std::string& msg, bool debug) {
    std::string debugLevel = debug ? ANSI_COLOR_GREEN "[DEBUG] " : "";
    std::cout << debugLevel << ANSI_COLOR_BLUE "[INFO]: " ANSI_COLOR_RESET << msg << std::endl;
}

void logger::warn(const std::string& msg, bool debug) {
    std::string debugLevel = debug ? ANSI_COLOR_GREEN "[DEBUG] " : "";
    std::cout << debugLevel << ANSI_COLOR_YELLOW "[WARN]: " ANSI_COLOR_RESET << msg << std::endl;
}

void logger::err(const std::string& msg, bool debug) {
    std::string debugLevel = debug ? ANSI_COLOR_GREEN "[DEBUG] " : "";
    std::cerr << debugLevel << ANSI_COLOR_RED "[ERROR]: " ANSI_COLOR_RESET << msg << std::endl;
}

#ifndef NDEBUG
void logger::validationLayers(const std::string& msg) {
    std::cout << ANSI_COLOR_PINK "[VALIDATION LAYERS]: " ANSI_COLOR_RESET << msg << std::endl;
}
#endif

} // namespace core