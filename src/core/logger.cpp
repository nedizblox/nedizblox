#include "logger.hpp"

#include <iostream>

#define ANSI_COLOR_BLUE "\x1b[94m"
#define ANSI_COLOR_YELLOW "\x1b[93m"
#define ANSI_COLOR_RED "\x1b[91m"

#ifndef NDEBUG
#define ANSI_COLOR_GREEN "\x1b[92m"
#endif

#define ANSI_COLOR_RESET "\x1b[39m"

namespace core {

void logger::info(const std::string& msg) {
    std::cout << ANSI_COLOR_BLUE "[INFO]: " ANSI_COLOR_RESET << msg << std::endl;
}

void logger::warn(const std::string& msg) {
    std::cout << ANSI_COLOR_YELLOW "[WARN]: " ANSI_COLOR_RESET << msg << std::endl;
}

void logger::err(const std::string& msg) {
    std::cerr << ANSI_COLOR_RED "[ERROR]: " ANSI_COLOR_RESET << msg << std::endl;
}

#ifndef NDEBUG
void logger::debug(const std::string& msg) {
    std::cout << ANSI_COLOR_GREEN "[DEBUG]: " ANSI_COLOR_RESET << msg << std::endl;
}
#endif

} // namespace core