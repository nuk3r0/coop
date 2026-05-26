#include "memory.h"

namespace Memory {

    MicroCache g_micro_cache;
    uintptr_t g_module_base = 0;
    uint32_t g_current_frame = 0;

    void Init() {
        g_module_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("GoWR.exe"));
        g_current_frame = 0;
        g_micro_cache.Flush();
    }

    void FlushMicroCache() {
        g_micro_cache.Flush();
    }

    void NextFrame() {
        g_current_frame++;
    }

    bool MicroCache::Get(CacheKey key, uint64_t& out, uint32_t frame) {
        auto it = map_.find(key);
        if (it == map_.end()) return false;
        if (it->second.frame != frame) {
            map_.erase(it);
            return false;
        }
        out = it->second.value;
        return true;
    }

    void MicroCache::Set(CacheKey key, uint64_t val, uint32_t frame) {
        map_[key] = {val, frame};
    }

    uintptr_t ResolveChain(uintptr_t rva, const std::vector<uintptr_t>& offsets) {
        if (!g_module_base) return 0;

        // Always dereference the base address to get the initial pointer
        uintptr_t ptr = Read<uintptr_t>(g_module_base + rva);
        if (!ptr) return 0;

        // Walk intermediate offsets (all except last are address dereferences)
        for (size_t i = 0; i + 1 < offsets.size(); ++i) {
            ptr = Read<uintptr_t>(ptr + offsets[i]);
            if (!ptr) return 0;
        }

        // Last offset (or only offset) — add as struct field offset, no dereference
        if (!offsets.empty())
            ptr += offsets.back();

        return ptr;
    }

    bool IsReadable(uintptr_t addr) {
        if (!addr) return false;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == 0)
            return false;
        return (mbi.State == MEM_COMMIT) &&
               (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ |
                               PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY));
    }

}
