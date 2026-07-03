#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace net::packets {

enum class RigMoveDirection : uint8_t {
    Forward = 0,
    Backward = 1,
    Left = 2,
    Right = 3,
    None = 4
};

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

// UDP

struct ClientUdpConnectPacket {
    uint32_t playerId = 0;
};

struct RigMovePacket {
    uint32_t playerId = 0;
    RigMoveDirection direction = RigMoveDirection::None;
    bool jump = false;
    float cameraPhi = 0.0f;
    glm::vec3 position{0.0f};
};

struct RigMoveBroadcastPacket {
    uint32_t playerId = 0;
    RigMoveDirection direction = RigMoveDirection::None;
    bool jump = false;
    float cameraPhi = 0.0f;
    glm::vec3 position{0.0f};
};

#pragma pack(pop)

} // namespace net::packets