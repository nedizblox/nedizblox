#pragma once

#include <string>

namespace core::msgbox {

void showInfo(const std::string& title, const std::string& message);
void showWarn(const std::string& title, const std::string& message);
void showError(const std::string& title, const std::string& message);

} // namespace core::msgbox