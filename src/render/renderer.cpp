#include "renderer.h"
#include "../utils/logger.h"
#include "../ui/ui_manager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <windows.h>
#include <cstdio>

#include "../core/runtime.h"
#include "../core/shutdown_state.h"

static bool g_init = false;
static bool g_d3d12Init = false;
static bool g_inert = false;

static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12CommandAllocator* g_pd3dCommandAlloc = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12Fence* g_gameFence = nullptr;
static ID3D12Fence* g_ourFence = nullptr;
static UINT64 g_gameFenceValue = 0;
static UINT64 g_ourFenceValue = 0;
static HANDLE g_ourFenceEvent = nullptr;
static UINT g_rtvDescriptorSize = 0;
static UINT g_srvDescriptorSize = 0;
static DXGI_FORMAT g_dxgiFormat = DXGI_FORMAT_UNKNOWN;
static ImGuiContext* g_context = nullptr;
static bool g_inside_render = false;

#define NUM_SRV_DESCRIPTORS 16
static int g_srvNextIndex = 0;

static void SrvDescriptorAllocFn(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
{
    int idx = (g_srvNextIndex++) % NUM_SRV_DESCRIPTORS;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += (SIZE_T)idx * g_srvDescriptorSize;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
    gpu.ptr += (SIZE_T)idx * g_srvDescriptorSize;
    *out_cpu = cpu;
    *out_gpu = gpu;
}

static void SrvDescriptorFreeFn(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
{
}

static void ReleaseD3D12Resources_NoWait() {
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dCommandAlloc) { g_pd3dCommandAlloc->Release(); g_pd3dCommandAlloc = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_ourFenceEvent) { CloseHandle(g_ourFenceEvent); g_ourFenceEvent = nullptr; }
    if (g_ourFence) { g_ourFence->Release(); g_ourFence = nullptr; }
    if (g_gameFence) { g_gameFence->Release(); g_gameFence = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
    g_d3d12Init = false;
    g_gameFenceValue = 0;
    g_ourFenceValue = 0;
}

void Renderer::EnterInertMode() {
    g_inert = true;
}

bool Renderer::TryFlushGPU(unsigned timeoutMs) {
    // Must be called outside Present hook.
    if (!g_d3d12Init) return true;
    if (!g_pd3dCommandQueue || !g_ourFence || !g_ourFenceEvent) return true;

    const UINT64 flushValue = g_ourFenceValue + 1;
    HRESULT hr = g_pd3dCommandQueue->Signal(g_ourFence, flushValue);
    if (FAILED(hr)) {
        LOG_WARNING("[Shutdown] GPU flush Signal failed (hr=0x%08X)", (unsigned)hr);
        return false;
    }

    g_ourFenceValue = flushValue;
    if (g_ourFence->GetCompletedValue() >= g_ourFenceValue) return true;

    g_ourFence->SetEventOnCompletion(g_ourFenceValue, g_ourFenceEvent);
    DWORD w = WaitForSingleObject(g_ourFenceEvent, timeoutMs);
    if (w == WAIT_TIMEOUT) {
        LOG_WARNING("[Shutdown] GPU flush timeout after %u ms", timeoutMs);
        return false;
    }

    return true;
}

bool Renderer::ReleaseResources_NoWait() {
    // Must only be called when the coordinator decided it's safe.
    // No internal flush/waits allowed here.
    if (!g_init) return true;

    g_inert = true;

    // Backends may touch DX12 on shutdown (driver dependent). Caller must ensure safety.
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_context = nullptr;

    ReleaseD3D12Resources_NoWait();
    g_init = false;
    return true;
}

bool Renderer::Initialize() {
    IMGUI_CHECKVERSION();
    g_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(g_context);
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    g_inert = false;
    g_init = true;
    LOG_INFO("Renderer context created");
    return true;
}

bool Renderer::IsInitialized() { return g_init; }

static bool InitD3D12(IDXGISwapChain* pSwapChain, ID3D12CommandQueue* pQueue) {
    if (FAILED(pQueue->GetDevice(IID_PPV_ARGS(&g_pd3dDevice)))) return false;

    DXGI_SWAP_CHAIN_DESC sd = {};
    if (FAILED(pSwapChain->GetDesc(&sd))) {
        LOG_ERROR("Failed to get swap chain desc");
        return false;
    }
    g_dxgiFormat = sd.BufferDesc.Format;
    UINT bufferCount = sd.BufferCount > 1 ? sd.BufferCount : 2;

    LOG_INFO("D3D12 init: format=%d, bufferCount=%u", (int)g_dxgiFormat, bufferCount);

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = NUM_SRV_DESCRIPTORS;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)))) return false;
    g_srvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = bufferCount;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)))) return false;
    g_rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(g_pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pd3dCommandQueue)))) return false;

    if (FAILED(g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pd3dCommandAlloc)))) return false;
    if (FAILED(g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pd3dCommandAlloc, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)))) return false;
    g_pd3dCommandList->Close(); // close so we can Reset later

    if (FAILED(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_gameFence)))) return false;
    if (FAILED(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_ourFence)))) return false;
    g_ourFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    HWND hwnd = nullptr;
    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
        DWORD pid = 0;
        GetWindowThreadProcessId(hWnd, &pid);
        if (pid == GetCurrentProcessId() && IsWindowVisible(hWnd)) {
            *(HWND*)lParam = hWnd;
            return FALSE;
        }
        return TRUE;
    }, (LPARAM)&hwnd);

    if (!hwnd) {
        EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
            DWORD pid = 0;
            GetWindowThreadProcessId(hWnd, &pid);
            if (pid == GetCurrentProcessId()) {
                *(HWND*)lParam = hWnd;
                return FALSE;
            }
            return TRUE;
        }, (LPARAM)&hwnd);
    }

    if (!hwnd) {
        hwnd = GetDesktopWindow();
    }

    LOG_INFO("Using HWND=%p for ImGui", hwnd);
    ImGui_ImplWin32_Init(hwnd);
    ::LoadLibraryA("d3dcompiler_47.dll");

    g_srvNextIndex = 0;
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = g_pd3dDevice;
    initInfo.CommandQueue = g_pd3dCommandQueue;
    initInfo.NumFramesInFlight = bufferCount;
    initInfo.RTVFormat = g_dxgiFormat;
    initInfo.SrvDescriptorHeap = g_pd3dSrvDescHeap;
    initInfo.SrvDescriptorAllocFn = SrvDescriptorAllocFn;
    initInfo.SrvDescriptorFreeFn = SrvDescriptorFreeFn;
    if (!ImGui_ImplDX12_Init(&initInfo)) {
        LOG_ERROR("ImGui_ImplDX12_Init failed");
        return false;
    }

    g_d3d12Init = true;
    LOG_INFO("D3D12 backend initialized");
    return true;
}

static void DebugLog(const char* msg) {
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

HRESULT Renderer::RenderFrame(IDXGISwapChain* pSwapChain, ID3D12CommandQueue* pQueue) {
    if (g_inert) {
        return S_OK;
    }

    if (Shutdown::StopRequested()) {
        return S_OK;
    }

    // Never do any real work while shutdown is in progress.
    if (Runtime::ShuttingDown().load(std::memory_order_acquire) ||
        !Runtime::Running().load(std::memory_order_acquire)) {
        return S_OK;
    }

    if (!g_init || !pSwapChain || !pQueue) return S_OK;
    if (g_inside_render) return S_OK;

    // If DX12 hasn't been initialized yet, only try init; don't proceed into rendering on the same call.
    // This avoids doing too much work in the first hooked Present.

    ImGui::SetCurrentContext(g_context);

    if (!g_d3d12Init) {
        DebugLog("[RF] InitD3D12 start");
        if (!InitD3D12(pSwapChain, pQueue)) return E_FAIL;
        DebugLog("[RF] InitD3D12 done");
        return S_OK;
    }

    g_inside_render = true;
    DebugLog("[RF] inside_render=true");

    if (Shutdown::StopRequested()) {
        g_inside_render = false;
        return S_OK;
    }

    // No blocking inside Present hook: if our previous work isn't done, just skip this frame.
    if (g_ourFence && g_ourFenceValue && g_ourFence->GetCompletedValue() < g_ourFenceValue) {
        g_inside_render = false;
        return S_OK;
    }

    DebugLog("[RF] Reset allocator");
    if (!g_pd3dCommandAlloc || !g_pd3dCommandList || !g_pd3dCommandQueue || !g_gameFence || !g_ourFence) {
        g_inside_render = false;
        return E_FAIL;
    }

    if (Shutdown::StopRequested()) {
        g_inside_render = false;
        return S_OK;
    }

    g_pd3dCommandAlloc->Reset();
    DebugLog("[RF] Reset command list");
    g_pd3dCommandList->Reset(g_pd3dCommandAlloc, nullptr);
    DebugLog("[RF] Game fence signal/wait");
    g_gameFenceValue++;
    (void)pQueue->Signal(g_gameFence, g_gameFenceValue);
    (void)g_pd3dCommandQueue->Wait(g_gameFence, g_gameFenceValue);
    DebugLog("[RF] entering __try");

    __try {
        DebugLog("[RF] __try start");

        if (Shutdown::StopRequested()) {
            g_inside_render = false;
            return S_OK;
        }

        UINT backBufferIndex = 0;
        IDXGISwapChain3* pSwapChain3 = nullptr;
        if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3)))) {
            backBufferIndex = pSwapChain3->GetCurrentBackBufferIndex();
            char buf[128];
            sprintf_s(buf, "[RF] backBufferIndex=%u", backBufferIndex);
            DebugLog(buf);
            pSwapChain3->Release();
        }

        ID3D12Resource* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&pBackBuffer)))) {
            DebugLog("[RF] GetBuffer FAILED");
            g_inside_render = false;
            return E_FAIL;
        }
        DebugLog("[RF] Got back buffer");

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += (SIZE_T)backBufferIndex * g_rtvDescriptorSize;
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
        DebugLog("[RF] Created RTV");

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pBackBuffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        g_pd3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        DebugLog("[RF] ImGui frames start");

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        UI::Render();
        ImGui::Render();

        DebugLog("[RF] ImGui render draws");
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        DebugLog("[RF] Draw data done");

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        g_pd3dCommandList->Close();
        DebugLog("[RF] List closed");

        ID3D12CommandList* lists[] = { g_pd3dCommandList };
        g_pd3dCommandQueue->ExecuteCommandLists(1, lists);
        DebugLog("[RF] Executed command lists");

        g_ourFenceValue++;
        g_pd3dCommandQueue->Signal(g_ourFence, g_ourFenceValue);
        DebugLog("[RF] Our fence signaled");

        // NOTE: Do NOT call pQueue->Wait() here.
        // Calling Wait on the game's queue from inside Present hook has caused hangs/crashes on some drivers.
        // We rely on executing on our own queue and keeping it lightweight.

        pBackBuffer->Release();
        g_inside_render = false;
        DebugLog("[RF] RenderFrame SUCCESS");
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        char buf[128];
        sprintf_s(buf, "[RF] CRASHED code 0x%08X", GetExceptionCode());
        DebugLog(buf);
        LOG_ERROR("RenderFrame CRASHED with code 0x%08X", GetExceptionCode());
        g_inside_render = false;
        return E_FAIL;
    }
}
