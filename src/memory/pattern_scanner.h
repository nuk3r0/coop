#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace PatternScan {

    enum class ResolveType {
        Direct,
        RipRel,
        ReadU32,
        Offset
    };

    struct SigEntry {
        const char* pattern;        // IDA-style: "48 8B 05 ?? ?? ?? ?? 48 85 C0"
        const char* mask;           // legacy: "xxxxxxxxxx"
        ResolveType resolve;
        uint32_t disp_offset;       // for RipRel: offset in bytes to the 4-byte displacement
        uint32_t instr_len;         // for RipRel: total instruction length
        int32_t delta;              // for Offset: signed delta from match
    };

    // Parse IDA-style hex pattern into bytes + mask
    // Returns pair<bytes, mask> where mask byte = 'x' (match) or '?' (wildcard)
    std::pair<std::vector<uint8_t>, std::string> ParsePattern(const char* pattern_str);

    // Legacy InModule scan (mask-based)
    uintptr_t InModule(const char* moduleName, const char* pattern, const char* mask);

    // Scan a byte buffer for a pattern with mask
    uintptr_t ScanBuffer(const uint8_t* data, size_t size,
                         const uint8_t* pattern, const char* mask);

    // RIP-relative resolution: reads i32 at match + disp_offset, returns match + instr_len + disp
    uintptr_t ResolveRipRel(uintptr_t match_addr, uint32_t disp_offset, uint32_t instr_len);

    // Full scan + resolve for a SigEntry
    uintptr_t FindAndResolve(const char* moduleName, const SigEntry& entry);

    // Get the virtual table pointer (first 8 bytes of an object)
    uint64_t ReadVTable(uintptr_t ptr);

}
