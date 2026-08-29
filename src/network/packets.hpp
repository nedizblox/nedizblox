#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <cstring>

namespace net::packets {

enum class PacketType : uint8_t {
    PlayerInfo = 1,
    PlayerAccept = 2,
    PlayerJoined = 3,
    PlayerLeft = 4,
    PlayerKick = 5
};

enum class PartType : uint8_t { Part = 0, MeshPart = 1, SpawnLocation = 2 };

enum class PartShape : uint8_t { Ball = 0, Block = 1, Cylinder = 2, Wedge = 3, Head = 4 };

#pragma pack(push, 1)

// TCP

struct PacketHeader {
    PacketType type;
};

struct PlayerInfoPacket {
    PacketHeader header{PacketType::PlayerInfo};
    uint32_t clientVersion = 0;
    char nickname[16] = {};
};

struct PlayerAcceptPacket {
    PacketHeader header{PacketType::PlayerAccept};
    uint32_t playerId = 0;
    uint64_t sessionToken = 0;
};

struct OldPlayerInfo {
    uint32_t playerId = 0;
    char nickname[16] = {};
    glm::vec3 position{0.0f};
};

struct InitialStatePacket {
    uint32_t playerCount = 0;
};

struct PlayerJoinedPacket {
    PacketHeader header{PacketType::PlayerJoined};
    uint32_t playerId = 0;
    char nickname[16] = {};
};

struct PlayerLeftPacket {
    PacketHeader header{PacketType::PlayerLeft};
    uint32_t playerId = 0;
};

struct PlayerKickPacket {
    PacketHeader header{PacketType::PlayerKick};
    char reason[256] = {};
};

struct TcpMessageHeader {
    uint32_t size = 0;
};

// end TCP

// map TCP

struct PartInfo {
    uint32_t networkId = 0;
    uint32_t parentNetworkId = 0;
    char name[100] = {};
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 size{4.0f, 1.0f, 2.0f};
    glm::u8vec3 color{160, 165, 169};
    float transparency = 0.0f;
    bool anchored = false;
    PartType type = PartType::Part;
    PartShape shape = PartShape::Block;
};

struct InitialMapPacket {
    uint32_t partCount = 0;
};

struct DestroyInstancePacket {
    uint32_t networkId;
};

// end map TCP

// UDP

struct ClientUdpConnectPacket {
    uint32_t playerId = 0;
    uint64_t sessionToken = 0;
};

struct PhysicalObjectState {
    uint32_t networkId = 0;
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 linearVelocity{0.0f};
    glm::vec3 angularVelocity{0.0f};
};

struct PhysicsStepPacket {
    uint32_t senderPlayerId = 0;
    uint32_t objectCount = 0;
};

struct PhysicalObjectStateWire {
    uint32_t networkId = 0;
    uint16_t position[3] = {0, 0, 0};
    uint32_t rotation = 0;
    uint16_t linearVelocity[3] = {0, 0, 0};
    uint16_t angularVelocity[3] = {0, 0, 0};
};

inline constexpr uint32_t kPingMagic = 0x50494e47;
inline constexpr uint32_t kPongMagic = 0x504f4e47;

struct PingPacket {
    uint32_t magic = kPingMagic;
    uint64_t clientTimeUs = 0;
};

struct PongPacket {
    uint32_t magic = kPongMagic;
    uint64_t clientTimeUs = 0;
};

// end UDP

#pragma pack(pop)

inline uint16_t floatToHalf(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));

    uint32_t sign = (bits >> 16) & 0x8000u;
    int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xFFu) - 127 + 15;
    uint32_t mantissa = bits & 0x7FFFFFu;

    if (((bits >> 23) & 0xFFu) == 0xFFu) {
        return static_cast<uint16_t>(sign | 0x7C00u | (mantissa ? 0x200u : 0u));
    }

    if (exponent <= 0) {
        return static_cast<uint16_t>(sign);
    }

    if (exponent >= 0x1F) {
        return static_cast<uint16_t>(sign | 0x7C00u);
    }

    return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exponent) << 10) | (mantissa >> 13));
}

inline float halfToFloat(uint16_t half) {
    uint32_t sign = static_cast<uint32_t>(half & 0x8000u) << 16;
    uint32_t exponent = (half >> 10) & 0x1Fu;
    uint32_t mantissa = half & 0x3FFu;

    uint32_t bits;
    if (exponent == 0) {
        bits = sign;
    } else if (exponent == 0x1F) {
        bits = sign | 0x7F800000u | (mantissa << 13);
    } else {
        bits = sign | ((exponent - 15 + 127) << 23) | (mantissa << 13);
    }

    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

inline uint32_t packQuat(const glm::quat& q) {
    float components[4] = {q.x, q.y, q.z, q.w};

    int largestIndex = 0;
    float largestValue = std::fabs(components[0]);
    for (int i = 1; i < 4; i++) {
        float value = std::fabs(components[i]);
        if (value > largestValue) {
            largestValue = value;
            largestIndex = i;
        }
    }

    float sign = (components[largestIndex] < 0.0f) ? -1.0f : 1.0f;

    constexpr float kMaxComponent = 0.70710678f;
    constexpr uint32_t kMaxQuantized = (1u << 10) - 1;

    uint32_t packed = static_cast<uint32_t>(largestIndex) << 30;

    int shift = 20;
    for (int i = 0; i < 4; i++) {
        if (i == largestIndex)
            continue;

        float normalized = (components[i] * sign / kMaxComponent) * 0.5f + 0.5f;
        normalized = normalized < 0.0f ? 0.0f : (normalized > 1.0f ? 1.0f : normalized);

        uint32_t quantized = static_cast<uint32_t>(normalized * static_cast<float>(kMaxQuantized) + 0.5f);

        packed |= (quantized & kMaxQuantized) << shift;
        shift -= 10;
    }

    return packed;
}

inline glm::quat unpackQuat(uint32_t packed) {
    constexpr float kMaxComponent = 0.70710678f;
    constexpr uint32_t kMaxQuantized = (1u << 10) - 1;

    int largestIndex = static_cast<int>((packed >> 30) & 0x3u);

    float components[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float sumSquares = 0.0f;

    int shift = 20;
    for (int i = 0; i < 4; i++) {
        if (i == largestIndex)
            continue;

        uint32_t quantized = (packed >> shift) & kMaxQuantized;
        shift -= 10;

        float normalized = static_cast<float>(quantized) / static_cast<float>(kMaxQuantized);
        float value = (normalized - 0.5f) * 2.0f * kMaxComponent;

        components[i] = value;
        sumSquares += value * value;
    }

    components[largestIndex] = glm::sqrt(glm::max(0.0f, 1.0f - sumSquares));

    return glm::quat(components[3], components[0], components[1], components[2]);
}

inline PhysicalObjectStateWire toWire(const PhysicalObjectState& state) {
    PhysicalObjectStateWire wire{};
    wire.networkId = state.networkId;

    wire.position[0] = floatToHalf(state.position.x);
    wire.position[1] = floatToHalf(state.position.y);
    wire.position[2] = floatToHalf(state.position.z);

    wire.rotation = packQuat(state.rotation);

    wire.linearVelocity[0] = floatToHalf(state.linearVelocity.x);
    wire.linearVelocity[1] = floatToHalf(state.linearVelocity.y);
    wire.linearVelocity[2] = floatToHalf(state.linearVelocity.z);

    wire.angularVelocity[0] = floatToHalf(state.angularVelocity.x);
    wire.angularVelocity[1] = floatToHalf(state.angularVelocity.y);
    wire.angularVelocity[2] = floatToHalf(state.angularVelocity.z);

    return wire;
}

inline PhysicalObjectState fromWire(const PhysicalObjectStateWire& wire) {
    PhysicalObjectState state{};
    state.networkId = wire.networkId;

    state.position = glm::vec3(
        halfToFloat(wire.position[0]), halfToFloat(wire.position[1]), halfToFloat(wire.position[2]));

    state.rotation = unpackQuat(wire.rotation);

    state.linearVelocity = glm::vec3(
        halfToFloat(wire.linearVelocity[0]), halfToFloat(wire.linearVelocity[1]),
        halfToFloat(wire.linearVelocity[2]));

    state.angularVelocity = glm::vec3(
        halfToFloat(wire.angularVelocity[0]), halfToFloat(wire.angularVelocity[1]),
        halfToFloat(wire.angularVelocity[2]));

    return state;
}

} // namespace net::packets