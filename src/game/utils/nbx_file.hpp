#pragma once

#include "../types/types.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace game::utils::nbxf {

constexpr uint32_t kMagic = 0xA3F00;
constexpr uint16_t kVersion = 1;

void registerInstanceType(
    enums::InstanceType type,
    std::function<std::shared_ptr<types::Instance>()> factory
);

std::vector<std::shared_ptr<types::Instance>>
parseNbxl(const std::string& filePath);

std::vector<std::shared_ptr<types::Instance>>
parseNbxlBuffer(const uint8_t* data, size_t size, const std::string& source);

std::vector<uint8_t>
serializeNbxlBuffer(const std::vector<std::shared_ptr<types::Instance>>& roots);

std::vector<uint8_t>
serializeTree(const std::shared_ptr<types::Instance>& root);

std::shared_ptr<types::Instance>
instantiateTree(const std::vector<uint8_t>& bytes);

void saveNbxl(
    const std::string& filePath,
    const std::vector<std::shared_ptr<types::Instance>>& roots
);

} // namespace game::utils::nbxf