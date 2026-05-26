#include "pattern_scanner.h"
#include "memory.h"
#include <windows.h>
#include <psapi.h>
#include <cstring>
#include <cstdio>

#pragma comment(lib, "Psapi.lib")

namespace PatternScan {

    std::pair<std::vector<uint8_t>, std::string> ParsePattern(const char* pattern_str) {
        std::vector<uint8_t> bytes;
        std::string mask;
        char tok[4] = {};

        const char* p = pattern_str;
        while (*p) {
            if (*p == ' ') { p++; continue; }

            if (p[0] == '?' && (p[1] == '?' || p[1] == ' ' || p[1] == '\0')) {
                bytes.push_back(0x00);
                mask += '?';
                p += (*p == '?') ? 1 : 0;
                if (*p == '?') p++;
            } else {
                if (sscanf_s(p, "%2s", tok, (unsigned)sizeof(tok)) != 1) break;
                unsigned int val;
                if (sscanf_s(tok, "%2x", &val) == 1) {
                    bytes.push_back((uint8_t)val);
                    mask += 'x';
                }
                p += 2;
            }
        }
        return {bytes, mask};
    }

    uintptr_t ScanBuffer(const uint8_t* data, size_t size,
                         const uint8_t* pattern, const char* mask) {
        size_t pattern_len = strlen(mask);
        if (pattern_len == 0 || size < pattern_len) return 0;

        for (size_t i = 0; i <= size - pattern_len; i++) {
            bool found = true;
            for (size_t j = 0; j < pattern_len; j++) {
                if (mask[j] == 'x' && data[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) return i;
        }
        return 0;
    }

    uintptr_t InModule(const char* moduleName, const char* pattern, const char* mask) {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) return 0;

        MODULEINFO modInfo{};
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
            return 0;

        uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        size_t size = modInfo.SizeOfImage;
        return ScanBuffer(reinterpret_cast<const uint8_t*>(base), size,
                          reinterpret_cast<const uint8_t*>(pattern), mask);
    }

    uintptr_t ResolveRipRel(uintptr_t match_addr, uint32_t disp_offset, uint32_t instr_len) {
        int32_t disp = *reinterpret_cast<const int32_t*>(match_addr + disp_offset);
        return match_addr + instr_len + static_cast<intptr_t>(disp);
    }

    uintptr_t FindAndResolve(const char* moduleName, const SigEntry& entry) {
        // Determine if we have an IDA-style pattern or legacy mask
        bool has_spaces = strchr(entry.pattern, ' ') != nullptr;

        const char* mask;
        std::vector<uint8_t> parsed_bytes;
        std::string parsed_mask;
        const uint8_t* pattern_bytes;

        if (has_spaces) {
            auto parsed = ParsePattern(entry.pattern);
            parsed_bytes = parsed.first;
            parsed_mask = parsed.second;
            pattern_bytes = parsed_bytes.data();
            mask = parsed_mask.c_str();
        } else {
            // legacy: pattern is raw bytes
            pattern_bytes = reinterpret_cast<const uint8_t*>(entry.pattern);
            mask = entry.mask;
        }

        uintptr_t match = InModule(moduleName,
                                   reinterpret_cast<const char*>(pattern_bytes), mask);
        if (!match) return 0;

        switch (entry.resolve) {
            case ResolveType::Direct:
                return match;
            case ResolveType::RipRel:
                return ResolveRipRel(match, entry.disp_offset, entry.instr_len);
            case ResolveType::ReadU32:
                return Memory::Read<uint32_t>(match + entry.disp_offset);
            case ResolveType::Offset:
                return match + entry.delta;
        }
        return match;
    }

    uint64_t ReadVTable(uintptr_t ptr) {
        if (!ptr) return 0;
        return *reinterpret_cast<const uint64_t*>(ptr);
    }

}
