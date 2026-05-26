#include "game_state.h"
#include "../memory/memory.h"
#include "../resolver/resolver.h"
#include "../utils/logger.h"
#include <windows.h>

static Entity::EntitySystem g_entity_system;

namespace GameState {

    bool Init() {
        Memory::Init();
        OutputDebugStringA("[GoW_CoOp] Memory::Init OK\n");
        LOG_INFO("[GameState] Module base: 0x%llX",
                 (unsigned long long)Memory::g_module_base);

        g_entity_system.Init();
        LOG_INFO("[GameState] Entity system initialized");
        return true;
    }

    void Shutdown() {
    }

    void Update() {
        g_entity_system.Update();
    }

    Vec3 GetLocalPosition() {
        return g_entity_system.GetLocal().pos;
    }

    Vec3 GetRemotePosition() {
        return g_entity_system.GetGuest().pos;
    }

    uintptr_t GetLocalPtr() {
        return g_entity_system.GetLocalPtr();
    }

    Entity::EntitySystem& GetSystem() {
        return g_entity_system;
    }

    void SetGuestPtr(uintptr_t ptr) {
        g_entity_system.SetGuestPtr(ptr);
    }

    void SetGuestSoftId(uint64_t id) {
        g_entity_system.SetGuestSoftId(id);
    }

    uint64_t GetGuestSoftId() {
        return g_entity_system.GetGuestSoftId();
    }

}
