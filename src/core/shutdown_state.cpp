#include "shutdown_state.h"

#include "runtime.h"

namespace Shutdown {
    static std::atomic<ShutdownState> g_shutdownState{ ShutdownState::RUNNING };
    static std::atomic<int> g_presentActive{ 0 };
    static std::atomic<bool> g_presentBypass{ false };
    static std::atomic<bool> g_coordinatorStarted{ false };
    static std::atomic<ShutdownOutcome> g_outcome{ ShutdownOutcome::UNKNOWN };

    std::atomic<ShutdownState>& State() { return g_shutdownState; }

    bool IsRunning() {
        return g_shutdownState.load(std::memory_order_acquire) == ShutdownState::RUNNING;
    }

    bool StopRequested() {
        return g_shutdownState.load(std::memory_order_acquire) != ShutdownState::RUNNING;
    }

    void SetState(ShutdownState s) {
        g_shutdownState.store(s, std::memory_order_release);
    }

    void BeginShutdown() {
        ShutdownState expected = ShutdownState::RUNNING;
        g_shutdownState.compare_exchange_strong(expected, ShutdownState::STOP_REQUESTED, std::memory_order_acq_rel);
        // Make hkPresent forward-only ASAP.
        g_presentBypass.store(true, std::memory_order_release);
        Runtime::RequestShutdown();
    }

    bool StartCoordinator() {
        bool expected = false;
        return g_coordinatorStarted.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
    }

    void EnablePresentBypass() {
        g_presentBypass.store(true, std::memory_order_release);
    }

    bool PresentBypassEnabled() {
        return g_presentBypass.load(std::memory_order_acquire);
    }

    ShutdownOutcome Outcome() {
        return g_outcome.load(std::memory_order_acquire);
    }

    // Coordinator-only: set once.
    static bool SetOutcomeOnce(ShutdownOutcome v) {
        ShutdownOutcome expected = ShutdownOutcome::UNKNOWN;
        return g_outcome.compare_exchange_strong(expected, v, std::memory_order_acq_rel);
    }

    // Expose coordinator-only setter to this TU via friend-like pattern.
    bool CoordinatorSetOutcomeOnce(ShutdownOutcome v) {
        return SetOutcomeOnce(v);
    }

    void PresentEnter() {
        g_presentActive.fetch_add(1, std::memory_order_acq_rel);
    }

    void PresentLeave() {
        g_presentActive.fetch_sub(1, std::memory_order_acq_rel);
    }

    int PresentActiveCount() {
        return g_presentActive.load(std::memory_order_acquire);
    }
}
