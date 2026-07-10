#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace net::packets {

enum class PartShape : uint8_t { Ball = 0, Block = 1 };

#pragma pack(push, 1)

// TCP

struct PlayerInfoPacket {
    char nickname[16] = {};
};

struct PlayerAcceptPacket {
    uint32_t playerId = 0;
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
    uint32_t playerId = 0;
    char nickname[16] = {};
};

struct PlayerLeftPacket {
    uint32_t playerId = 0;
};

struct PlayerKickPacket {
    char reason[64] = {};
};

struct TcpMessageHeader {
    uint32_t size = 0;
};

struct MapPartInfo {
    uint32_t networkId = 0;
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 size{4.0f, 1.0f, 2.0f};
    glm::u8vec3 color{160, 165, 169};
    float transparency = 0.0f;
    bool anchored = false;
    PartShape shape = PartShape::Block;
};

struct InitialMapPacket {
    uint32_t partCount = 0;
};

// UDP

struct ClientUdpConnectPacket {
    uint32_t playerId = 0;
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

#pragma pack(pop)

} // namespace net::packets