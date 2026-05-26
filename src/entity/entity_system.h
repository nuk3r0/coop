#pragma once
#include <cstdint>
#include "../network/packet.h"

namespace Entity {

    struct PlayerSlot {
        uintptr_t ptr = 0;
        Vec3 pos{};
    };

    class EntitySystem {
        PlayerSlot local_;
        PlayerSlot guest_;
        uint64_t guest_soft_id_ = 0;

    public:
        void Init();
        void Update();                          // read positions from memory
        void SetGuestPtr(uintptr_t ptr);        // assign guest entity

        const PlayerSlot& GetLocal() const;
        const PlayerSlot& GetGuest() const;
        uint64_t GetGuestSoftId() const;
        void SetGuestSoftId(uint64_t id);
        uintptr_t GetLocalPtr() const;
    };

}
