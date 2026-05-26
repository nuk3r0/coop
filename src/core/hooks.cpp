#include "hooks.h"
#include "../utils/logger.h"
#include "../render/renderer.h"
#include <MinHook.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "runtime.h"
#include "shutdown_state.h"

// --- Typedefs ---
using Present_t = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using CreateCommandQueue_t = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMMAND_QUEUE_DESC*, REFIID, void**);

static Present_t oPresent = nullptr;
static CreateCommandQueue_t oCreateCommandQueue = nullptr;
static ID3D12CommandQueue* g_pGameCommandQueue = nullptr;

// --- Hooks ---
HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static bool g_device_lost = false;

    Shutdown::PresentEnter();

    // Ensure PresentActiveCount is decremented on every path.
    struct _LeaveGuard { ~_LeaveGuard() { Shutdown::PresentLeave(); } } leaveGuard;

    // Present hook MUST be state-aware: never execute overlay after shutdown begins.
    // Bypass makes the hook a pure forwarder even if the detour remains installed.
    if (Shutdown::PresentBypassEnabled() || !Shutdown::IsRunning()) {
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    // During shutdown we must not execute any rendering or blocking work.
    if (Runtime::ShuttingDown().load(std::memory_order_acquire) ||
        !Runtime::Running().load(std::memory_order_acquire)) {
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    Runtime::MarkPresent();

    // Attempt recovery if previously lost
    if (g_device_lost) {
        if (SUCCEEDED(ImGui_ImplDX12_CreateDeviceObjects())) {
            g_device_lost = false;
            LOG_INFO("[UI] Device recovered");
        }
        // If still lost, skip rendering this frame
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    // If we don't have the game's queue yet, just pass through.
    if (!g_pGameCommandQueue) {
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    HRESULT hr = Renderer::RenderFrame(pSwapChain, g_pGameCommandQueue);
    if (FAILED(hr)) {
        g_device_lost = true;
        ImGui_ImplDX12_InvalidateDeviceObjects();
        LOG_WARNING("[UI] Device lost - will attempt recovery");
    }
    return oPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE hkCreateCommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_QUEUE_DESC* pDesc, REFIID riid, void** ppCommandQueue) {
    HRESULT hr = oCreateCommandQueue(pDevice, pDesc, riid, ppCommandQueue);
    if (SUCCEEDED(hr) && ppCommandQueue && *ppCommandQueue) {
        if (!g_pGameCommandQueue && pDesc->Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            g_pGameCommandQueue = static_cast<ID3D12CommandQueue*>(*ppCommandQueue);
            g_pGameCommandQueue->AddRef();
            LOG_INFO("Captured ID3D12CommandQueue @ %p (Type=DIRECT)", g_pGameCommandQueue);
        }
    }
    return hr;
}

// --- Helpers to find vtable methods ---
static void* GetPresentAddress() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, DefWindowProcW, 0L, 0L, GetModuleHandleW(nullptr), NULL, NULL, NULL, NULL, L"TempClass", NULL };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Temp", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) { UnregisterClassW(wc.lpszClassName, wc.hInstance); return nullptr; }

    IDXGIFactory4* pFactory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory)))) {
        DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance); return nullptr;
    }

    ID3D12Device* pDevice = nullptr;
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)))) {
        pFactory->Release(); DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance); return nullptr;
    }

    ID3D12CommandQueue* pQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC desc = {};
    if (FAILED(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pQueue)))) {
        pDevice->Release(); pFactory->Release(); DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance); return nullptr;
    }

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = 100; sd.Height = 100;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain1* pSwapChain = nullptr;
    if (FAILED(pFactory->CreateSwapChainForHwnd(pQueue, hwnd, &sd, nullptr, nullptr, &pSwapChain))) {
        pQueue->Release(); pDevice->Release(); pFactory->Release(); DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance); return nullptr;
    }

    void** vtable = *(void***)pSwapChain;
    void* present = vtable[8];

    pSwapChain->Release();
    pQueue->Release();
    pDevice->Release();
    pFactory->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return present;
}

static void* GetCreateCommandQueueAddress() {
    ID3D12Device* pDevice = nullptr;
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)))) return nullptr;
    void** vtable = *(void***)pDevice;
    void* addr = vtable[8]; // CreateCommandQueue
    pDevice->Release();
    return addr;
}

// --- Public ---
bool Hooks::Install() {
    if (MH_Initialize() != MH_OK) return false;

    // Hook CreateCommandQueue first so we capture the game's queue
    void* pCreateCQ = GetCreateCommandQueueAddress();
    if (pCreateCQ) {
        if (MH_CreateHook(pCreateCQ, hkCreateCommandQueue, reinterpret_cast<void**>(&oCreateCommandQueue)) != MH_OK ||
            MH_EnableHook(pCreateCQ) != MH_OK) {
            LOG_ERROR("Failed to hook CreateCommandQueue");
        } else {
            LOG_INFO("CreateCommandQueue hooked @ %p", pCreateCQ);
        }
    }

    // Hook Present
    void* pPresent = GetPresentAddress();
    if (!pPresent) {
        LOG_ERROR("Failed to find Present address");
        return false;
    }
    LOG_INFO("Present found @ %p", pPresent);

    if (MH_CreateHook(pPresent, hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK ||
        MH_EnableHook(pPresent) != MH_OK) {
        LOG_ERROR("Failed to hook Present");
        return false;
    }

    LOG_INFO("Hooks installed");
    return true;
}

bool Hooks::Remove() {
    // Best-effort removal. If Present is stuck in driver, this must not hang.
    MH_STATUS st1 = MH_DisableHook(MH_ALL_HOOKS);
    MH_STATUS st2 = MH_Uninitialize();

    if (g_pGameCommandQueue) {
        g_pGameCommandQueue->Release();
        g_pGameCommandQueue = nullptr;
    }

    return st1 == MH_OK && st2 == MH_OK;
}
