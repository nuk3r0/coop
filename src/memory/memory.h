#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <windows.h>

namespace Memory {

    // Per-frame micro-cache entry
    struct CachedValue {
        uint64_t value;
        uint32_t frame;
    };

    // Per-frame cache key
    struct CacheKey {
        uintptr_t rva;
        uintptr_t offsets_hash;
        uint32_t type_hash;
        bool operator==(const CacheKey& o) const {
            return rva == o.rva && offsets_hash == o.offsets_hash && type_hash == o.type_hash;
        }
    };

    struct CacheKeyHash {
        size_t operator()(const CacheKey& k) const {
            return k.rva ^ (k.offsets_hash << 13) ^ (k.type_hash << 23);
        }
    };

    class MicroCache {
        std::unordered_map<CacheKey, CachedValue, CacheKeyHash> map_;
    public:
        void Flush() { map_.clear(); }
        bool Get(CacheKey key, uint64_t& out, uint32_t frame);
        void Set(CacheKey key, uint64_t val, uint32_t frame);
    };

    extern MicroCache g_micro_cache;
    extern uintptr_t g_module_base;
    extern uint32_t g_current_frame;

    void Init();
    void FlushMicroCache();
    void NextFrame();

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, T>::type
    Read(uintptr_t addr) {
        if (!addr) return T{};
        T val;
        __try {
            val = *reinterpret_cast<const T*>(addr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            memset(&val, 0, sizeof(T));
        }
        return val;
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, bool>::type
    Write(uintptr_t addr, T val) {
        if (!addr) return false;
        __try {
            *reinterpret_cast<T*>(addr) = val;
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    uintptr_t ResolveChain(uintptr_t rva, const std::vector<uintptr_t>& offsets);
    bool IsReadable(uintptr_t addr);

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value && sizeof(T) <= 8, T>::type
    ReadCached(uintptr_t rva, const std::vector<uintptr_t>& offsets) {
        static_assert(sizeof(T) <= 8, "ReadCached only supports types <= 8 bytes");
        CacheKey key;
        key.rva = rva;
        key.offsets_hash = 0;
        for (auto o : offsets) key.offsets_hash ^= o << 1;
        key.type_hash = (uint32_t)sizeof(T);

        uint64_t cached;
        if (g_micro_cache.Get(key, cached, g_current_frame))
            return *reinterpret_cast<const T*>(&cached);

        uintptr_t addr = ResolveChain(rva, offsets);
        T val = Read<T>(addr);

        uint64_t storage = 0;
        memcpy(&storage, &val, sizeof(T));
        g_micro_cache.Set(key, storage, g_current_frame);
        return val;
    }

}
