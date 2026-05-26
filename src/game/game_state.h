#pragma once
#include <cstdint>
#include "../network/packet.h"
#include "../entity/entity_system.h"

namespace GameState {

    bool Init();
    void Shutdown();
    void Update();

    Vec3 GetLocalPosition();
    Vec3 GetRemotePosition();
    uintptr_t GetLocalPtr();

    Entity::EntitySystem& GetSystem();

    // Network state helpers
    void SetGuestPtr(uintptr_t ptr);
    void SetGuestSoftId(uint64_t id);
    uint64_t GetGuestSoftId();

}
