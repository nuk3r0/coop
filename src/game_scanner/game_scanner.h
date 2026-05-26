#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace GameScanner {

    struct ScanResult {
        bool success = false;
        
        // Pointers
        uintptr_t local_player_ptr = 0;      // CE static pointer to local player
        uintptr_t locomotion_list = 0;       // g_list head
        
        // Entity offsets (relative to entity pointer)
        uint32_t offset_pos_x = 0;
        uint32_t offset_pos_y = 0;
        uint32_t offset_pos_z = 0;
        uint32_t offset_name = 0;
        uint32_t offset_wad = 0;
        uint32_t offset_hp = 0;
        uint32_t offset_speed = 0;
        
        // VTable for validation
        uint64_t expected_vtable = 0;
        
        std::string version_detected;
    };
    
    // Main scan function - call once at startup
    ScanResult ScanAll();
    
    // Individual scanners (for debugging/fallback)
    uintptr_t FindLocalPlayerPointer();
    uintptr_t FindLocomotionList();
    std::optional<uint32_t> FindPositionOffsets(uintptr_t entity_ptr);
    std::optional<uint32_t> FindNameOffset(uintptr_t entity_ptr);
    
    // Helper: verify entity is valid by checking vtable and position
    bool IsValidEntity(uintptr_t ptr);
    
    // Scan for common patterns in GoW
    namespace Patterns {
        // Local player pointer patterns (multiple versions)
        inline const char* LOCAL_PLAYER_PATTERNS[] = {
            "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 40 ?? C3",  // Common pattern
            "48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 41 ?? C3",  // Alternative
            "48 8B 15 ?? ?? ?? ?? 48 85 D2 74 ?? 48 8B 42 ?? C3"   // Another variant
        };
        
        // Locomotion list patterns
        inline const char* LOCOMOTION_LIST_PATTERNS[] = {
            "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 40 08",     // Common
            "48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 49 08"      // Alternative
        };
        
        // Position access patterns (to find offset)
        inline const char* POSITION_PATTERNS[] = {
            "F3 0F 10 8? ?? ?? ?? ??",  // movss xmm0, [rcx/rbx+offset]
            "F3 0F 11 8? ?? ?? ?? ??"   // movss [rcx/rbx+offset], xmm0
        };
    }
    
}
