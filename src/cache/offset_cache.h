#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Cache {
    struct CachedOffset {
        std::string pattern_name;
        uintptr_t address;
        int32_t offset;
        uint64_t timestamp;
        std::string game_version;
    };

    class OffsetCache {
    public:
        static OffsetCache& Instance();
        
        bool Load(const std::string& path);
        bool Save(const std::string& path);
        
        bool GetCachedOffset(const std::string& pattern_name, uintptr_t& address, int32_t& offset);
        void SetCachedOffset(const std::string& pattern_name, uintptr_t address, int32_t offset, const std::string& game_version);
        
        bool IsCacheValid(const std::string& game_version);
        void Clear();
        
        const std::unordered_map<std::string, CachedOffset>& GetAllOffsets() const { return m_offsets; }
        
    private:
        OffsetCache() = default;
        ~OffsetCache() = default;
        OffsetCache(const OffsetCache&) = delete;
        OffsetCache& operator=(const OffsetCache&) = delete;
        
        std::unordered_map<std::string, CachedOffset> m_offsets;
        std::string m_last_game_version;
        uint64_t m_cache_timestamp;
    };
}
