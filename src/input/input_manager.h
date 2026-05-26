#pragma once
#include <windows.h>
#include "../network/packet.h"

class InputManager {
public:
    static InputManager& Get() {
        static InputManager im;
        return im;
    }
    void Update();
    PlayerInput GetCurrentInput() const { return m_current; }
    bool IsMenuTogglePressed();
    bool IsNetworkTogglePressed();
private:
    PlayerInput m_current{};
    bool m_menuPrev = false;
    bool m_netPrev = false;
};
