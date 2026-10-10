#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace net::serializer {

class BinaryWriter {
public:
    std::vector<uint8_t> buffer;

    template <typename T>
    void write(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
    }

    void writeString(const std::string& str) {
        write<uint16_t>(static_cast<uint16_t>(str.size()));
        buffer.insert(buffer.end(), str.begin(), str.end());
    }
};

class BinaryReader {
public:
    const uint8_t* data;
    size_t size;
    size_t offset = 0;

    BinaryReader(const uint8_t* data, size_t size) : data(data), size(size) {}

    template <typename T>
    T read() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        if (offset + sizeof(T) > size) {
            throw std::runtime_error("BinaryReader overflow read!");
        }
        T value;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    std::string readString() {
        auto length = read<uint16_t>();
        if (offset + length > size) {
            throw std::runtime_error("BinaryReader overflow read string");
        }
        std::string str(reinterpret_cast<const char*>(data + offset), length);
        offset += length;
        return str;
    }
};

} // namespace net::serializer