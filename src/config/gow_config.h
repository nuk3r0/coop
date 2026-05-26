#pragma once
#include <cstdint>
#include <vector>

struct GoWConfig {
    const char* game_version = "1.0.668.5700";

    // ── Local player (CE static pointer) ──────────────────────
    uintptr_t local_player_rva    = 0x295F200;     // points directly to entity

    // ── Pointer chains ────────────────────────────────────────
    uintptr_t locomotion_list_rva = 0x24F2F20;     // locomotion::g_list

    // ── Player struct offsets (Aug 2025 build) ─────────────────
    uintptr_t pos_x             = 0x3E0;
    uintptr_t pos_z             = 0x3E4;
    uintptr_t pos_y             = 0x3E8;
    uintptr_t hp                = 0x0A0;
    uintptr_t speed             = 0x0DC;
    uintptr_t name_offset       = 0x0C8;
    uintptr_t wad_offset        = 0x0F0;
    uintptr_t teleport_trigger  = 0x0BC;
    float direct_threshold  = 500.0f;
    float velocity_threshold = 50.0f;

    // ── Runtime state ─────────────────────────────────────────
    uint64_t expected_vtable = 0;
};

extern GoWConfig g_gow_config;
