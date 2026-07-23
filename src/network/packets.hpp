#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace net::packets {

enum class PacketType : uint8_t {
    PlayerInfo = 1,
    PlayerAccept = 2,
    PlayerJoined = 3,
    PlayerLeft = 4,
    PlayerKick = 5
};

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

#pragma pack(pop)

} // namespace net::packets