#pragma once

namespace core::hash {

template <typename T, typename... Rest>
void combine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (combine(seed, rest), ...);
};

} // namespace core::hash