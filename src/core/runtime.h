#pragma once

#include <atomic>
#include <cstdint>

// Simple global runtime state shared across threads/hooks.
namespace Runtime {
    // True while the mod is allowed to run work. Set false to begin shutdown.
    std::atomic<bool>& Running();

    // True once shutdown is requested (latched).
    std::atomic<bool>& ShuttingDown();

    // Idempotent: marks shutdown requested and stops Running().
    void RequestShutdown();

    // Updated by hkPresent.
    void MarkPresent();

    // Milliseconds since steady_clock epoch of last Present observed.
    uint64_t LastPresentMs();
}
