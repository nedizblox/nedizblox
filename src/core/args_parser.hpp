#pragma once

#include <unordered_map>
#include <string>

namespace core::argsparser {

std::unordered_map<std::string, std::string> parseArguments(int argc, char* argv[]);

} // namespace core::argsparser