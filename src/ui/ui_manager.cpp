#include "ui_manager.h"
#include "imgui.h"
#include "../network/network_manager.h"
#include "../game/game_state.h"
#include "../memory/memory.h"
#include "../utils/logger.h"

static bool g_showMenu  = false;
static bool g_showNet   = false;

void UI::ToggleMenu() { g_showMenu = !g_showMenu; }
void UI::ToggleNetworkWindow() { g_showNet = !g_showNet; }
bool UI::IsMenuOpen() { return g_showMenu; }
bool UI::IsNetworkWindowOpen() { return g_showNet; }

void UI::Render() {
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.92f);

    if (g_showMenu) {
        ImGui::SetNextWindowSize(ImVec2(520, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("GoW Co-Op v3", &g_showMenu, ImGuiWindowFlags_NoCollapse)) {
            auto& es = GameState::GetSystem();
            auto& local = es.GetLocal();
            auto& guest = es.GetGuest();

            ImGui::Text("Local ptr: 0x%llX", (unsigned long long)local.ptr);
            if (local.ptr)
                ImGui::Text("Local pos: (%.2f, %.2f, %.2f)",  local.pos.x, local.pos.y, local.pos.z);
            else
                ImGui::Text("Local: NOT RESOLVED");

            ImGui::Separator();
            ImGui::Text("Guest ptr: 0x%llX", (unsigned long long)guest.ptr);
            ImGui::Text("Guest soft_id: 0x%llX", (unsigned long long)es.GetGuestSoftId());
            if (guest.ptr)
                ImGui::Text("Guest pos: (%.2f, %.2f, %.2f)", guest.pos.x, guest.pos.y, guest.pos.z);
            else
                ImGui::Text("Guest: NONE");

            ImGui::Separator();
            auto& nm = NetworkManager::Get();
            ImGui::Text("Role: %s", nm.GetRole() == NetRole::Host ? "HOST" :
                                    nm.GetRole() == NetRole::Client ? "CLIENT" : "NONE");
            ImGui::Text("State: %s", nm.GetState() == NetState::Connected ? "Connected" :
                                     nm.GetState() == NetState::Listening ? "Listening" : "Disconnected");
            ImGui::Text("Latency: %.2f ms", nm.GetStats().latencyMs);
            ImGui::Text("Pkts S=%u R=%u", nm.GetStats().packetsSent, nm.GetStats().packetsRecv);
        }
        ImGui::End();
    }

    if (g_showNet) {
        ImGui::SetNextWindowSize(ImVec2(420, 160), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Network (F8)", &g_showNet, ImGuiWindowFlags_NoCollapse)) {
            static char ipBuf[64] = "127.0.0.1";
            static int port = 7777;
            static bool hostMode = false;

            ImGui::InputText("IP", ipBuf, IM_ARRAYSIZE(ipBuf));
            ImGui::InputInt("Port", &port);
            ImGui::Checkbox("Host Mode", &hostMode);

            if (ImGui::Button(hostMode ? "Start Host" : "Connect", ImVec2(120, 25))) {
                if (hostMode) {
                    NetworkManager::Get().StartHost(port);
                } else {
                    NetworkManager::Get().Connect(ipBuf, port);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Disconnect", ImVec2(120, 25))) {
                NetworkManager::Get().Disconnect();
            }

            auto& nm = NetworkManager::Get();
            ImGui::Text("Role: %s State: %d",
                        nm.GetRole() == NetRole::Host ? "Host" :
                        nm.GetRole() == NetRole::Client ? "Client" : "None",
                        (int)nm.GetState());
        }
        ImGui::End();
    }

    ImGui::PopStyleVar();
}
