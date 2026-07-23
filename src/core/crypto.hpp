#pragma once

#include <cstdint>

namespace core::crypto {

uint64_t generateToken();

constexpr uint32_t encodeVersion(uint32_t major, uint32_t minor, uint32_t patch) {
    return (major << 16) | (minor << 8) | patch;
}

constexpr void decodeVersion(uint32_t version, uint32_t& major, uint32_t& minor, uint32_t& patch) {
    major = (version >> 16) & 0xFF;
    minor = (version >> 8) & 0xFF;
    patch = version & 0xFF;
}

} // namespace core::crypto