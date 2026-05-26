#include <windows.h>
#include <thread>
#include <atomic>
#include <string>
#include <cmath>
#include <chrono>

#include "../memory/memory.h"
#include "../config/gow_config.h"
#include "../resolver/resolver.h"
#include "../replication/replication.h"
#include "../game/game_state.h"

#include "../core/hooks.h"
#include "../render/renderer.h"
#include "../ui/ui_manager.h"
#include "../network/network_manager.h"
#include "../input/input_manager.h"
#include "../utils/logger.h"
#include "../utils/config.h"

#include "runtime.h"
#include "shutdown_state.h"

static Config g_config;

static std::string GetDllDir(HMODULE hModule) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    if (pos != std::wstring::npos) ws.resize(pos + 1);
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string dir(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &dir[0], len, nullptr, nullptr);
    dir.pop_back();
    return dir;
}

void ModThread(HMODULE hModule) {
    OutputDebugStringA("[GoW_CoOp] ModThread started\n");

    std::string dllDir = GetDllDir(hModule);
    std::string logPath = dllDir + "GoW_CoOp.log";
    std::string cfgPath = dllDir + "GoW_CoOp.json";

    Logger::Get().Initialize(logPath);
    LOG_INFO("GoW Co-Op Mod v3 — Simple Co-op");
    OutputDebugStringA("[GoW_CoOp] Logger OK\n");

    g_config.Load(cfgPath);
    OutputDebugStringA("[GoW_CoOp] Config OK\n");

    if (!GameState::Init()) {
        LOG_ERROR("[Main] GameState::Init failed");
        return; // Do not rely on forced DLL unload for correctness.
    }
    OutputDebugStringA("[GoW_CoOp] GameState OK\n");

    NetworkManager::Get().Initialize();
    OutputDebugStringA("[GoW_CoOp] Network OK\n");

    Replication::Init();
    LOG_INFO("[Main] Core subsystems ready");

    bool ui_inited = false;

    if (g_config.isHost && g_config.coopEnabled) {
        NetworkManager::Get().StartHost(g_config.lastPort);
    }

    while (Runtime::Running().load(std::memory_order_acquire)) {
        // If the game window is no longer valid, assume shutdown.
        // This avoids the mod thread keeping the process alive.
        static HWND s_gameHwnd = nullptr;
        static int s_missingTicks = 0;
        if (!s_gameHwnd || !IsWindow(s_gameHwnd)) {
            // Find a top-level window belonging to this process.
            s_gameHwnd = nullptr;
            EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
                DWORD pid = 0;
                GetWindowThreadProcessId(hWnd, &pid);
                if (pid == GetCurrentProcessId()) {
                    *reinterpret_cast<HWND*>(lParam) = hWnd;
                    return FALSE;
                }
                return TRUE;
            }, (LPARAM)&s_gameHwnd);
        }
        if (!s_gameHwnd || !IsWindow(s_gameHwnd)) {
            if (++s_missingTicks > 120) { // ~2 seconds
                LOG_INFO("[Shutdown] Game window missing -> requesting shutdown");
                Shutdown::BeginShutdown();
                break;
            }
        } else {
            s_missingTicks = 0;
        }

        // Lazy UI init (after game is running to avoid DX12 hangs in main menu)
        if (!ui_inited) {
            Renderer::Initialize();
            if (Hooks::Install()) {
                UI::ToggleMenu();
                ui_inited = true;
                LOG_INFO("[Main] UI initialized");
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        // Do not auto-shutdown based on Present cadence.
        // GoWR can legitimately stop presenting (loading screens, focus changes), and we must not tear down hooks/resources mid-run.

        Memory::FlushMicroCache();
        GameState::Update();

        if (NetworkManager::Get().GetRole() == NetRole::Host) {
            Replication::HostTick();
        } else if (NetworkManager::Get().GetRole() == NetRole::Client) {
            Replication::ClientTick();
        }

        NetworkManager::Get().Update();
        InputManager::Get().Update();
        if (InputManager::Get().IsMenuTogglePressed()) UI::ToggleMenu();
        if (InputManager::Get().IsNetworkTogglePressed()) UI::ToggleNetworkWindow();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // --- Shutdown coordinator (Phase 1 -> stop/inert, Phase 2 optional attempt) ---
    Shutdown::BeginShutdown();
    Shutdown::EnablePresentBypass();
    LOG_INFO("[Shutdown] STOP_REQUESTED");

    if (!Shutdown::StartCoordinator()) {
        // Another context already started shutdown pipeline.
        return;
    }

    LOG_INFO("[Shutdown] render thread exiting");
    Shutdown::SetState(ShutdownState::RENDER_STOPPED);

    // Phase 1: deterministic safe shutdown (no DX12 calls here)
    LOG_INFO("[Shutdown] stopping network");
    NetworkManager::Get().Shutdown();
    Renderer::EnterInertMode();

    // Best-effort Present drain barrier (execution safety only)
    constexpr int PRESENT_DRAIN_TIMEOUT_MS = 10000;
    LOG_INFO("[Shutdown] waiting for Present drain (active=%d)", Shutdown::PresentActiveCount());
    const auto t0 = std::chrono::steady_clock::now();
    bool drained = false;
    while (Shutdown::PresentActiveCount() > 0) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() > PRESENT_DRAIN_TIMEOUT_MS) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    drained = (Shutdown::PresentActiveCount() == 0);

    if (!drained) {
        LOG_WARNING("[Shutdown] Present drain TIMEOUT (active=%d) -> INERT_ONLY exit path", Shutdown::PresentActiveCount());
        // After timeout: DO NOT remove hooks, DO NOT flush GPU, DO NOT touch DX12.
        (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::INERT_ONLY);
        LOG_INFO("[Shutdown] outcome=INERT_ONLY");
        LOG_INFO("[Shutdown] process exit safe");
        GameState::Shutdown();
        g_config.Save(cfgPath);
        Logger::Get().Shutdown();
        return;
    }

    LOG_INFO("[Shutdown] Present drained");

    // Phase 2 eligibility is execution-safety only (no DX12/device checks)
    const bool phase2Eligible = (Shutdown::PresentActiveCount() == 0);
    if (!phase2Eligible) {
        (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::INERT_ONLY);
        LOG_INFO("[Shutdown] outcome=INERT_ONLY (phase2 not eligible)");
        LOG_INFO("[Shutdown] process exit safe");
        GameState::Shutdown();
        g_config.Save(cfgPath);
        Logger::Get().Shutdown();
        return;
    }

    // Phase 2: optional speculative cleanup attempt (fail-fast)
    LOG_INFO("[Shutdown] Phase2 attempt start");

    // Step 1: hook removal attempt (optional). Failure is not fatal for correctness.
    const bool hooksRemoved = Hooks::Remove();
    if (hooksRemoved) {
        Shutdown::SetState(ShutdownState::HOOKS_REMOVED);
        LOG_INFO("[Shutdown] hooks removed");
    } else {
        LOG_WARNING("[Shutdown] hooks removal skipped/failed (bypass active)");
    }

    // Step 2: attempt GPU flush (best-effort). Failure -> fallback.
    LOG_INFO("[Shutdown] GPU flush start");
    if (!Renderer::TryFlushGPU(4000)) {
        LOG_WARNING("[Shutdown] Phase2 anomaly: GPU flush TIMEOUT/FAIL -> INERT_ONLY");
        Shutdown::SetState(ShutdownState::GPU_FLUSHED);
        (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::INERT_ONLY);
        LOG_INFO("[Shutdown] outcome=INERT_ONLY");
        LOG_INFO("[Shutdown] process exit safe");
        GameState::Shutdown();
        g_config.Save(cfgPath);
        Logger::Get().Shutdown();
        return;
    }
    Shutdown::SetState(ShutdownState::GPU_FLUSHED);
    LOG_INFO("[Shutdown] GPU flush success");

    // Step 3: ultra-safe release attempt (optional).
    // Release can still block driver-side. Only attempt it when hooks were removed.
    if (!hooksRemoved) {
        LOG_INFO("[Shutdown] release skipped (hooks not removed)");
        (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::INERT_ONLY);
        LOG_INFO("[Shutdown] outcome=INERT_ONLY");
        LOG_INFO("[Shutdown] process exit safe");
        GameState::Shutdown();
        g_config.Save(cfgPath);
        Logger::Get().Shutdown();
        return;
    }

    // Require PresentActiveCount to stay at 0 for a short window before releasing.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (Shutdown::PresentActiveCount() != 0) {
        LOG_WARNING("[Shutdown] Phase2 anomaly: Present became active again -> INERT_ONLY");
        (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::INERT_ONLY);
        LOG_INFO("[Shutdown] outcome=INERT_ONLY");
        LOG_INFO("[Shutdown] process exit safe");
        GameState::Shutdown();
        g_config.Save(cfgPath);
        Logger::Get().Shutdown();
        return;
    }

    // Release is a best-effort attempt; any anomaly -> INERT_ONLY.
    if (!Renderer::ReleaseResources_NoWait()) {
        LOG_WARNING("[Shutdown] Phase2 anomaly: resource release failed -> INERT_ONLY");
        (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::INERT_ONLY);
        LOG_INFO("[Shutdown] outcome=INERT_ONLY");
        LOG_INFO("[Shutdown] process exit safe");
        GameState::Shutdown();
        g_config.Save(cfgPath);
        Logger::Get().Shutdown();
        return;
    }

    Shutdown::SetState(ShutdownState::RESOURCES_RELEASED);
    (void)Shutdown::CoordinatorSetOutcomeOnce(ShutdownOutcome::FULL_TEARDOWN);
    LOG_INFO("[Shutdown] resources released");
    LOG_INFO("[Shutdown] outcome=FULL_TEARDOWN");

    GameState::Shutdown();
    g_config.Save(cfgPath);

    LOG_INFO("[Shutdown] process exit safe");
    Logger::Get().Shutdown();
    return;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE h = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ModThread, hModule, 0, nullptr);
        if (h) CloseHandle(h);
    } else if (reason == DLL_PROCESS_DETACH) {
        // Keep this very lightweight (loader lock). Just request shutdown so hooks stop executing.
        Shutdown::BeginShutdown();
    }
    return TRUE;
}
