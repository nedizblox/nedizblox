#pragma once

#include <string>

namespace core::logger {

void info(const std::string& msg, bool debug = false);
void warn(const std::string& msg, bool debug = false);
void err(const std::string& msg, bool debug = false);

#ifndef NDEBUG
void validationLayers(const std::string& msg);
#endif

} // namespace core::logger