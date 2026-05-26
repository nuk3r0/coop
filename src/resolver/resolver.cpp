#include "resolver.h"
#include "../memory/memory.h"
#include "../config/gow_config.h"
#include "../utils/logger.h"
#include <cstring>
#include <unordered_set>
#include <cmath>

namespace Resolver {

    uintptr_t ResolveLocalPlayer() {
        if (!Memory::g_module_base) return 0;

        uintptr_t addr = Memory::g_module_base + g_gow_config.local_player_rva;
        uintptr_t ptr = Memory::Read<uintptr_t>(addr);
        if (!ptr || !Memory::IsReadable(ptr)) return 0;

        uint64_t vtable = Memory::Read<uint64_t>(ptr);
        if (!vtable || vtable == 0xFFFFFFFFFFFFFFFF) return 0;

        float px = Memory::Read<float>(ptr + g_gow_config.pos_x);
        if (!std::isfinite(px)) return 0;

        if (!g_gow_config.expected_vtable)
            g_gow_config.expected_vtable = vtable;

        return ptr;
    }

    std::vector<uintptr_t> TraverseGList() {
        std::vector<uintptr_t> entities;
        if (!Memory::g_module_base) return entities;

        uintptr_t list_head = Memory::ResolveChain(
            g_gow_config.locomotion_list_rva, {}
        );
        if (!list_head) return entities;

        uintptr_t current = Memory::Read<uintptr_t>(list_head);
        if (!current) return entities;

        std::unordered_set<uintptr_t> seen;
        int max_iter = 500;

        while (current && seen.find(current) == seen.end() && max_iter-- > 0) {
            seen.insert(current);

            uint64_t vtable = Memory::Read<uint64_t>(current);
            if (vtable != 0 && vtable != 0xFFFFFFFFFFFFFFFF)
                entities.push_back(current);

            uintptr_t next = Memory::Read<uintptr_t>(current + 0x08);
            if (next == current || next == 0) break;
            current = next;
        }

        return entities;
    }

    static bool SafeReadStr(uintptr_t addr, char* buf, size_t len) {
        __try {
            strncpy_s(buf, len, reinterpret_cast<const char*>(addr), len - 1);
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    std::string ReadName(uintptr_t entity_ptr) {
        if (!entity_ptr) return "";
        uintptr_t addr = entity_ptr + g_gow_config.name_offset;
        if (!Memory::IsReadable(addr)) return "";

        char buf[64] = {};
        if (!SafeReadStr(addr, buf, sizeof(buf)))
            return "";
        return std::string(buf);
    }

    Vec3 ReadPos(uintptr_t entity_ptr) {
        Vec3 pos{};
        if (!entity_ptr) return pos;
        pos.x = Memory::Read<float>(entity_ptr + g_gow_config.pos_x);
        pos.y = Memory::Read<float>(entity_ptr + g_gow_config.pos_y);
        pos.z = Memory::Read<float>(entity_ptr + g_gow_config.pos_z);
        return pos;
    }

    uint64_t CalcSoftId(uintptr_t entity_ptr) {
        if (!entity_ptr) return 0;

        std::string name = ReadName(entity_ptr);
        uintptr_t wad = Memory::Read<uintptr_t>(entity_ptr + g_gow_config.wad_offset);

        uint64_t hash = 5381;
        for (char c : name)
            hash = ((hash << 5) + hash) + (uint8_t)c;
        hash ^= (uint64_t)wad << 13;
        hash ^= (uint64_t)wad >> 19;
        return hash;
    }

    static bool IsValidGuest(uintptr_t ptr, uintptr_t local_ptr) {
        if (!ptr || ptr == local_ptr) return false;
        if (!Memory::IsReadable(ptr)) return false;

        uint64_t vtable = Memory::Read<uint64_t>(ptr);
        if (!vtable || vtable == 0xFFFFFFFFFFFFFFFF) return false;

        Vec3 pos = ReadPos(ptr);
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z))
            return false;

        return true;
    }

    uintptr_t PickGuestEntity(uintptr_t local_ptr) {
        if (!local_ptr) return 0;

        auto entities = TraverseGList();
        for (uintptr_t e : entities) {
            if (IsValidGuest(e, local_ptr))
                return e;
        }
        return 0;
    }

    uintptr_t FindEntityBySoftId(uint64_t soft_id) {
        if (!soft_id) return 0;

        auto entities = TraverseGList();
        for (uintptr_t e : entities) {
            if (CalcSoftId(e) == soft_id)
                return e;
        }
        return 0;
    }

}
