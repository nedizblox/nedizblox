#include "signal_handler.hpp"

#include "logger.hpp"

#include <csignal>
#include <format>

namespace core {

bool sighandler::shouldStop = false;

static void handleSignal(int sig) {
    logger::info(std::format("Catched signal {}, stopping", sig));
    sighandler::shouldStop = true;
}

void sighandler::listen() {
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
}

} // namespace core