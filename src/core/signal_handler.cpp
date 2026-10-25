#include "signal_handler.hpp"

#include "logger.hpp"

#include <csignal>

namespace core {

bool sighandler::shouldStop = false;

static void handleSignal(int sig) {
    logger::info("Catched signal ", sig, ", stopping");
    sighandler::shouldStop = true;
}

void sighandler::listen() {
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
}

} // namespace core