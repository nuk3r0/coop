#pragma once

#include <atomic>

enum class ShutdownState {
    RUNNING = 0,
    STOP_REQUESTED,
    RENDER_STOPPED,
    HOOKS_REMOVED,
    GPU_FLUSHED,
    RESOURCES_RELEASED,
};

enum class ShutdownOutcome {
    UNKNOWN = 0,
    FULL_TEARDOWN,
    INERT_ONLY,
};

namespace Shutdown {
    // Global shutdown state machine.
    std::atomic<ShutdownState>& State();

    // Single entry point.
    void BeginShutdown();

    // Coordinator ownership: only the shutdown coordinator may run Phase 2 and set outcome.
    bool StartCoordinator();

    // Present hook bypass: when enabled, hkPresent must forward only.
    void EnablePresentBypass();
    bool PresentBypassEnabled();

    // Outcome is diagnostic only (read-only outside coordinator).
    ShutdownOutcome Outcome();

    // Coordinator-only: set once. Do not call from hooks/renderer/worker threads.
    bool CoordinatorSetOutcomeOnce(ShutdownOutcome v);

    // Helpers.
    bool IsRunning();
    bool StopRequested();
    void SetState(ShutdownState s);

    // Present hook activity tracking (used as an exit barrier).
    void PresentEnter();
    void PresentLeave();
    int  PresentActiveCount();
}
