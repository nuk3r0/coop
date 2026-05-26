#include "runtime.h"

#include <chrono>

namespace Runtime {
    static std::atomic<bool> g_running{ true };
    static std::atomic<bool> g_shuttingDown{ false };
    static std::atomic<uint64_t> g_lastPresentMs{ 0 };

    std::atomic<bool>& Running() { return g_running; }
    std::atomic<bool>& ShuttingDown() { return g_shuttingDown; }

    void RequestShutdown() {
        bool expected = false;
        if (g_shuttingDown.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            g_running.store(false, std::memory_order_release);
        } else {
            g_running.store(false, std::memory_order_release);
        }
    }

    void MarkPresent() {
        using namespace std::chrono;
        const uint64_t now = (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        g_lastPresentMs.store(now, std::memory_order_relaxed);
    }

    uint64_t LastPresentMs() {
        return g_lastPresentMs.load(std::memory_order_relaxed);
    }
}
