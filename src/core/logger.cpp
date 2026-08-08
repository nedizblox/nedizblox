#include "logger.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

#ifdef PLATFORM_WINDOWS
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#define ANSI_COLOR_BLUE "\x1b[94m"
#define ANSI_COLOR_YELLOW "\x1b[93m"
#define ANSI_COLOR_RED "\x1b[91m"
#define ANSI_COLOR_GREEN "\x1b[92m"

#ifndef NDEBUG
#define ANSI_COLOR_PINK "\x1b[95m"
#endif

#define ANSI_COLOR_RESET "\x1b[39m"

bool g_supportsAnsi = false;

bool checkAnsiSupport() {
#if defined(PLATFORM_WINDOWS)
    if (!_isatty(_fileno(stdout))) {
        return false;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(hOut, dwMode)) {
        return true;
    }

    return false;
#else
    if (!isatty(fileno(stdout))) {
        return false;
    }

    const char* term = std::getenv("TERM");
    if (term && std::strcmp(term, "dumb") == 0) {
        return false;
    }

    return true;
#endif
}

namespace core {

void logger::init() {
#if defined(PLATFORM_WINDOWS)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
    }
#endif

    std::ios_base::sync_with_stdio(true);

    g_supportsAnsi = checkAnsiSupport();
}

void logger::info(const std::string& msg, bool debug) {
    if (g_supportsAnsi) {
        std::string debugLevel = debug ? ANSI_COLOR_GREEN "[DEBUG] " : "";
        std::cout << debugLevel << ANSI_COLOR_BLUE "[INFO]: " ANSI_COLOR_RESET << msg << std::endl;
    } else {
        std::string debugLevel = debug ? "[DEBUG] " : "";
        std::cout << debugLevel << "[INFO]: " << msg << std::endl;
    }
}

void logger::warn(const std::string& msg, bool debug) {
    if (g_supportsAnsi) {
        std::string debugLevel = debug ? ANSI_COLOR_GREEN "[DEBUG] " : "";
        std::cout << debugLevel << ANSI_COLOR_YELLOW "[WARN]: " ANSI_COLOR_RESET << msg << std::endl;
    } else {
        std::string debugLevel = debug ? "[DEBUG] " : "";
        std::cout << debugLevel << "[WARN]: " << msg << std::endl;
    }
}

void logger::err(const std::string& msg, bool debug) {
    if (g_supportsAnsi) {
        std::string debugLevel = debug ? ANSI_COLOR_GREEN "[DEBUG] " : "";
        std::cerr << debugLevel << ANSI_COLOR_RED "[ERROR]: " ANSI_COLOR_RESET << msg << std::endl;
    } else {
        std::string debugLevel = debug ? "[DEBUG] " : "";
        std::cerr << debugLevel << "[ERROR]: " << msg << std::endl;
    }
}

#ifndef NDEBUG
void logger::validationLayers(const std::string& msg) {
    if (g_supportsAnsi) {
        std::cout << ANSI_COLOR_PINK "[VALIDATION LAYERS]: " ANSI_COLOR_RESET << msg << std::endl;
    } else {
        std::cout << "[VALIDATION LAYERS]: " << msg << std::endl;
    }
}
#endif

} // namespace core