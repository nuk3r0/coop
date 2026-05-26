#pragma once
#include <cstdint>

#pragma pack(push, 1)

enum class PacketType : uint8_t {
    ConnectionRequest = 0,
    ConnectionAccepted,
    Heartbeat,
    Input,
    WorldState,
    Event,
    PlayerSnapshot
};

struct PacketHeader {
    uint32_t magic = 0x434F5747; // 'GOWC'
    PacketType type;
    uint32_t sequence;
    uint32_t timestamp; // ms
};

struct Vec3 { float x, y, z; };
struct Quat { float x, y, z, w; };

struct PlayerSnapshot {
    PacketHeader header;
    Vec3 pos;
};

struct PlayerInput {
    uint16_t buttons; // bitmask
    float camYaw;
    float camPitch;
};

struct PlayerNetState {
    Vec3 pos;
    Quat rot;
    float animTime;
    uint32_t health;
    uint32_t maxHealth;
};

struct ConnectionRequestPacket {
    PacketHeader header;
    char padding[16];
};

struct ConnectionAcceptedPacket {
    PacketHeader header;
    uint32_t clientId;
    uint64_t guestSoftId;   // identity of guest entity for client
};

struct InputPacket {
    PacketHeader header;
    PlayerInput input;
};

struct WorldStatePacket {
    PacketHeader header;
    uint32_t tick;
    PlayerNetState hostPlayer;
    PlayerNetState clientPlayer;
    uint64_t guestSoftId;   // identity of guest entity (set by host)
    uint32_t enemyCount;
};

struct EventPacket {
    PacketHeader header;
    uint32_t eventId;
    uint32_t targetId;
    float damage;
};
#pragma pack(pop)
