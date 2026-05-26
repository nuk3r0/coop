#include "entity_system.h"
#include "../memory/memory.h"
#include "../resolver/resolver.h"
#include "../config/gow_config.h"

namespace Entity {

    void EntitySystem::Init() {
        local_ = {};
        guest_ = {};
        guest_soft_id_ = 0;
    }

    void EntitySystem::Update() {
        // Update local player
        uintptr_t lp = Resolver::ResolveLocalPlayer();
        if (lp) {
            local_.ptr = lp;
            local_.pos = Resolver::ReadPos(lp);
        } else {
            local_.ptr = 0;
            local_.pos = {};
        }

        // Update guest
        if (guest_.ptr) {
            if (Memory::IsReadable(guest_.ptr)) {
                guest_.pos = Resolver::ReadPos(guest_.ptr);
            } else {
                // Guest entity disappeared
                guest_.ptr = 0;
                guest_.pos = {};
            }
        }
    }

    void EntitySystem::SetGuestPtr(uintptr_t ptr) {
        guest_.ptr = ptr;
        if (ptr)
            guest_.pos = Resolver::ReadPos(ptr);
        else
            guest_.pos = {};
    }

    const PlayerSlot& EntitySystem::GetLocal() const { return local_; }
    const PlayerSlot& EntitySystem::GetGuest() const { return guest_; }
    uint64_t EntitySystem::GetGuestSoftId() const { return guest_soft_id_; }
    void EntitySystem::SetGuestSoftId(uint64_t id) { guest_soft_id_ = id; }
    uintptr_t EntitySystem::GetLocalPtr() const { return local_.ptr; }

}
