#include "input_manager.h"

void InputManager::Update() {
    m_current.buttons = 0;
    auto key = [](int v) -> bool { return (GetAsyncKeyState(v) & 0x8000) != 0; };

    if (key('W')) m_current.buttons |= (1 << 0);
    if (key('A')) m_current.buttons |= (1 << 1);
    if (key('S')) m_current.buttons |= (1 << 2);
    if (key('D')) m_current.buttons |= (1 << 3);
    if (key(VK_SPACE)) m_current.buttons |= (1 << 4);
    if (key(VK_LSHIFT)) m_current.buttons |= (1 << 5);
    if (key(VK_LBUTTON)) m_current.buttons |= (1 << 6);
    if (key(VK_RBUTTON)) m_current.buttons |= (1 << 7);

    // TODO: Read mouse delta via Raw Input or hook to fill camYaw / camPitch
    m_current.camYaw = 0.0f;
    m_current.camPitch = 0.0f;
}

bool InputManager::IsMenuTogglePressed() {
    bool pressed = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
    bool ret = pressed && !m_menuPrev;
    m_menuPrev = pressed;
    return ret;
}

bool InputManager::IsNetworkTogglePressed() {
    bool pressed = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    bool ret = pressed && !m_netPrev;
    m_netPrev = pressed;
    return ret;
}
