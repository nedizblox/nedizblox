#pragma once

#include <string>

namespace core::logger {

void info(const std::string& msg);
void warn(const std::string& msg);
void err(const std::string& msg);

#ifndef NDEBUG
void debug(const std::string& msg);
#endif

} // namespace core::logger