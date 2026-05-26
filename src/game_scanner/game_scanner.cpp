#include "game_scanner.h"
#include "../memory/pattern_scanner.h"
#include "../memory/memory.h"
#include "../utils/logger.h"
#include <windows.h>
#include <psapi.h>
#include <cmath>
#include <algorithm>

namespace GameScanner {

    bool IsValidEntity(uintptr_t ptr) {
        if (!ptr || !Memory::IsReadable(ptr)) return false;
        
        uint64_t vtable = Memory::Read<uint64_t>(ptr);
        if (!vtable || vtable == 0xFFFFFFFFFFFFFFFF) return false;
        
        // Check if position is finite (try common offsets)
        for (uint32_t offset : {0x3E0, 0x3D0, 0x3C0, 0x3B0}) {
            float x = Memory::Read<float>(ptr + offset);
            if (std::isfinite(x) && std::abs(x) < 10000.0f) {
                return true;
            }
        }
        return false;
    }

    uintptr_t FindLocalPlayerPointer() {
        if (!Memory::g_module_base) return 0;
        
        // Try multiple patterns
        for (const char* pattern : Patterns::LOCAL_PLAYER_PATTERNS) {
            auto [bytes, mask] = PatternScan::ParsePattern(pattern);
            
            HMODULE hModule = GetModuleHandleA(nullptr);
            MODULEINFO modInfo{};
            if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
                continue;
            
            uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
            size_t size = modInfo.SizeOfImage;
            
            uintptr_t match = PatternScan::ScanBuffer(
                reinterpret_cast<const uint8_t*>(base), size,
                bytes.data(), mask.c_str()
            );
            
            if (match) {
                match += base;
                // Resolve RIP-relative
                int32_t disp = *reinterpret_cast<const int32_t*>(match + 3);
                uintptr_t result = match + 7 + static_cast<intptr_t>(disp);
                
                if (Memory::IsReadable(result)) {
                    uintptr_t ptr = Memory::Read<uintptr_t>(result);
                    if (ptr && IsValidEntity(ptr)) {
                        LOG_INFO("Found local player via pattern at 0x%llX -> 0x%llX", 
                                 (unsigned long long)match, (unsigned long long)ptr);
                        return ptr;
                    }
                }
            }
        }
        
        LOG_WARNING("Local player pattern scan failed");
        return 0;
    }

    uintptr_t FindLocomotionList() {
        if (!Memory::g_module_base) return 0;
        
        for (const char* pattern : Patterns::LOCOMOTION_LIST_PATTERNS) {
            auto [bytes, mask] = PatternScan::ParsePattern(pattern);
            
            HMODULE hModule = GetModuleHandleA(nullptr);
            MODULEINFO modInfo{};
            if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
                continue;
            
            uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
            size_t size = modInfo.SizeOfImage;
            
            uintptr_t match = PatternScan::ScanBuffer(
                reinterpret_cast<const uint8_t*>(base), size,
                bytes.data(), mask.c_str()
            );
            
            if (match) {
                match += base;
                int32_t disp = *reinterpret_cast<const int32_t*>(match + 3);
                uintptr_t result = match + 7 + static_cast<intptr_t>(disp);
                
                if (Memory::IsReadable(result)) {
                    LOG_INFO("Found locomotion list at 0x%llX", (unsigned long long)result);
                    return result;
                }
            }
        }
        
        LOG_WARNING("Locomotion list pattern scan failed");
        return 0;
    }

    std::optional<uint32_t> FindPositionOffsets(uintptr_t entity_ptr) {
        if (!entity_ptr) return std::nullopt;
        
        // Common position offsets in games
        std::vector<std::pair<uint32_t, uint32_t>> test_offsets = {
            {0x3E0, 0x3E8}, // GoW default
            {0x3D0, 0x3D8},
            {0x3C0, 0x3C8},
            {0x3B0, 0x3B8},
            {0x400, 0x408},
            {0x410, 0x418}
        };
        
        for (auto [off_x, off_y] : test_offsets) {
            float x = Memory::Read<float>(entity_ptr + off_x);
            float y = Memory::Read<float>(entity_ptr + off_y);
            float z = Memory::Read<float>(entity_ptr + off_x + 4);
            
            if (std::isfinite(x) && std::isfinite(y) && std::isfinite(z)) {
                // Check if values are reasonable for game coordinates
                if (std::abs(x) < 10000.0f && std::abs(y) < 10000.0f && std::abs(z) < 10000.0f) {
                    LOG_INFO("Found position offsets: X=0x%X, Y=0x%X, Z=0x%X", 
                             off_x, off_y, off_x + 4);
                    return off_x;
                }
            }
        }
        
        return std::nullopt;
    }

    std::optional<uint32_t> FindNameOffset(uintptr_t entity_ptr) {
        if (!entity_ptr) return std::nullopt;
        
        // Common name offsets
        std::vector<uint32_t> test_offsets = {0x0C8, 0x0D0, 0x0C0, 0x0D8, 0x0E0};
        
        for (uint32_t offset : test_offsets) {
            uintptr_t str_ptr = Memory::Read<uintptr_t>(entity_ptr + offset);
            if (str_ptr && Memory::IsReadable(str_ptr)) {
                // Try to read first few chars
                char buf[8] = {};
                __try {
                    for (int i = 0; i < 7; i++) {
                        char c = *reinterpret_cast<char*>(str_ptr + i);
                        if (c >= 32 && c < 127) {
                            buf[i] = c;
                        } else if (c == 0) {
                            break;
                        } else {
                            break;
                        }
                    }
                    
                    if (strlen(buf) >= 3) {
                        LOG_INFO("Found name at offset 0x%X: '%s'", offset, buf);
                        return offset;
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    continue;
                }
            }
        }
        
        return std::nullopt;
    }

    ScanResult ScanAll() {
        ScanResult result;
        
        LOG_INFO("[GameScanner] Starting automatic scan...");
        
        // Find local player
        result.local_player_ptr = FindLocalPlayerPointer();
        if (!result.local_player_ptr) {
            LOG_ERROR("[GameScanner] Failed to find local player");
            return result;
        }
        
        // Read vtable
        result.expected_vtable = Memory::Read<uint64_t>(result.local_player_ptr);
        
        // Find position offsets
        auto pos_off = FindPositionOffsets(result.local_player_ptr);
        if (pos_off) {
            result.offset_pos_x = *pos_off;
            result.offset_pos_z = *pos_off + 4;
            result.offset_pos_y = *pos_off + 8;
        } else {
            // Fallback to default
            result.offset_pos_x = 0x3E0;
            result.offset_pos_z = 0x3E4;
            result.offset_pos_y = 0x3E8;
            LOG_WARNING("[GameScanner] Using default position offsets");
        }
        
        // Find name offset
        auto name_off = FindNameOffset(result.local_player_ptr);
        if (name_off) {
            result.offset_name = *name_off;
        } else {
            result.offset_name = 0x0C8;
            LOG_WARNING("[GameScanner] Using default name offset");
        }
        
        // Default offsets
        result.offset_wad = 0x0F0;
        result.offset_hp = 0x0A0;
        result.offset_speed = 0x0DC;
        
        // Find locomotion list
        result.locomotion_list = FindLocomotionList();
        
        result.success = true;
        result.version_detected = "Auto-detected";
        
        LOG_INFO("[GameScanner] Scan complete:");
        LOG_INFO("  Local player: 0x%llX", (unsigned long long)result.local_player_ptr);
        LOG_INFO("  VTable: 0x%llX", (unsigned long long)result.expected_vtable);
        LOG_INFO("  Position: X=0x%X, Y=0x%X, Z=0x%X", 
                 result.offset_pos_x, result.offset_pos_y, result.offset_pos_z);
        LOG_INFO("  Name offset: 0x%X", result.offset_name);
        LOG_INFO("  Locomotion list: 0x%llX", (unsigned long long)result.locomotion_list);
        
        return result;
    }

}
