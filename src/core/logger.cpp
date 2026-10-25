#include "logger.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <chrono>
#include <iomanip>

#if defined(PLATFORM_WINDOWS)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace core {

bool logger::g_supportsAnsi = false;

void logger::init() {
#if defined(PLATFORM_WINDOWS)
    if (!_isatty(_fileno(stdout))) {
        g_supportsAnsi = false;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        g_supportsAnsi = false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        g_supportsAnsi = false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(hOut, dwMode)) {
        g_supportsAnsi = true;
    }

    g_supportsAnsi = false;
#else
    if (!isatty(fileno(stdout))) {
        g_supportsAnsi = false;
    }

    const char* term = std::getenv("TERM");
    if (term && std::strcmp(term, "dumb") == 0) {
        g_supportsAnsi = false;
    }

    g_supportsAnsi = true;
#endif
}

std::string logger::getTime() {
    std::time_t t =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::tm ltime;
    localtime_r(&t, &ltime);

    std::stringstream ss;
    ss << std::put_time(&ltime, "%H:%M:%S");

    return ss.str();
}

} // namespace core