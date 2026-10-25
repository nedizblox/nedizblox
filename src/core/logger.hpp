#pragma once

#define ANSI_COLOR_BLUE "\x1b[94m"
#define ANSI_COLOR_YELLOW "\x1b[93m"
#define ANSI_COLOR_RED "\x1b[91m"
#define ANSI_COLOR_GREEN "\x1b[92m"
#define ANSI_COLOR_GREY "\x1b[90m"
#define ANSI_COLOR_RESET "\x1b[39m"

#include <iostream>

namespace core::logger {

extern bool g_supportsAnsi;

void init();

std::string getTime();

template<typename... Args>
void info(Args... args) {
    if (g_supportsAnsi) {
        std::cout << ANSI_COLOR_GREY "[" << getTime() << "] "
                  << ANSI_COLOR_BLUE "[INFO]: " ANSI_COLOR_RESET;
    } else {
        std::cout << "[" << getTime() << "] " << "[INFO]: ";
    }
    (std::cout << ... << args) << '\n';
}

template<typename... Args>
void warn(Args... args) {
    if (g_supportsAnsi) {
        std::cout << ANSI_COLOR_GREY "[" << getTime() << "] "
                  << ANSI_COLOR_YELLOW "[WARN]: " ANSI_COLOR_RESET;
    } else {
        std::cout << "[" << getTime() << "] " << "[WARN]: ";
    }
    (std::cout << ... << args) << '\n';
}

template<typename... Args>
void err(Args... args) {
    if (g_supportsAnsi) {
        std::cout << ANSI_COLOR_GREY "[" << getTime() << "] "
                  << ANSI_COLOR_RED "[ERROR]: " ANSI_COLOR_RESET;
    } else {
        std::cout << "[" << getTime() << "] " << "[ERROR]: ";
    }
    (std::cout << ... << args) << '\n';
}

template<typename... Args>
void debug(Args... args) {
    if (g_supportsAnsi) {
        std::cout << ANSI_COLOR_GREY "[" << getTime() << "] "
                  << ANSI_COLOR_GREEN "[DEBUG]: " ANSI_COLOR_RESET;
    } else {
        std::cout << "[" << getTime() << "] " << "[DEBUG]: ";
    }
    (std::cout << ... << args) << '\n';
}

} // namespace core::logger