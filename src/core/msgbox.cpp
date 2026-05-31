#include "msgbox.hpp"

#include <boxer/boxer.h>

namespace core {

void msgbox::showInfo(const std::string& title, const std::string& message) {
    boxer::show(message.c_str(), title.c_str(), boxer::Style::Info);
}

void msgbox::showWarn(const std::string& title, const std::string& message) {
    boxer::show(message.c_str(), title.c_str(), boxer::Style::Warning);
}

void msgbox::showError(const std::string& title, const std::string& message) {
    boxer::show(message.c_str(), title.c_str(), boxer::Style::Error);
}

} // namespace core