#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>

namespace Renderer {
    bool Initialize();
    
    // Runtime detach: disables any future overlay work.
    void EnterInertMode();

    // Phase 2 helpers (must NOT be called from Present hook).
    // Returns true if fence completed within timeout.
    bool TryFlushGPU(unsigned timeoutMs);

    // Release resources without internal waits/flush.
    // Must only be called when coordinator decided it's safe.
    bool ReleaseResources_NoWait();
    
    // Called automatically from hkPresent once D3D12 is ready
    HRESULT RenderFrame(IDXGISwapChain* pSwapChain, ID3D12CommandQueue* pQueue);
    
    bool IsInitialized();
}
