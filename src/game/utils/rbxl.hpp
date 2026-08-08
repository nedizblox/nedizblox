#pragma once

#include "../types/types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace game::utils::rbxl {

std::vector<std::shared_ptr<types::Instance>> parseRbxl(const std::string& filePath);

} // namespace game::utils::rbxl