#include "ui_manager.h"
#include "imgui.h"
#include "../network/network_manager.h"
#include "../game/game_state.h"
#include "../memory/memory.h"
#include "../utils/logger.h"
#include <cmath>

static bool g_showMenu  = false;
static bool g_showNet   = false;

void UI::ToggleMenu() { g_showMenu = !g_showMenu; }
void UI::ToggleNetworkWindow() { g_showNet = !g_showNet; }
bool UI::IsMenuOpen() { return g_showMenu; }
bool UI::IsNetworkWindowOpen() { return g_showNet; }

// Modern style setup
static void SetupModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors - Dark modern theme with accent
    ImVec4* colors = style.Colors;
    
    // Base colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.95f);
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    
    // Accent color (blue-purple gradient feel)
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.24f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.35f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.40f, 1.0f);
    
    // Frame backgrounds
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.26f, 1.0f);
    
    // Text
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.0f);
    
    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.30f, 0.5f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    
    // Special elements
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.16f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.24f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.14f, 0.8f);
    
    // Scrollbars
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
    
    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.30f, 0.5f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.35f, 0.40f, 0.8f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
    
    // Tab
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.16f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.26f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.32f, 1.0f);
    
    // Plot lines
    colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.70f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.80f, 0.80f, 0.90f, 1.0f);
    
    // Popup
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.95f);
    
    // Checkmark
    colors[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.55f, 0.85f, 1.0f);
    
    // Slider grab
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.55f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.55f, 0.70f, 1.0f);
    
    // Nav highlight
    colors[ImGuiCol_NavHighlight] = ImVec4(0.35f, 0.35f, 0.50f, 1.0f);
    
    // Separators
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
    
    // Style settings for rounded modern look
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.PopupRounding = 8.0f;
    style.ChildRounding = 8.0f;
    
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    
    style.WindowPadding = ImVec2(12, 10);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ItemInnerSpacing = ImVec2(6, 6);
    
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 12.0f;
}

// Helper to draw colored status
static void DrawStatusBadge(const char* label, ImVec4 color) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
}

// Overload with format support
static void DrawStatusBadge(const char* format, ImVec4 color, float value) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text(format, value);
    ImGui::PopStyleColor();
}

// Helper to format bytes nicely
static void DrawDataField(const char* label, const char* value, bool dim = false) {
    ImGui::TextColored(dim ? ImVec4(0.5f, 0.5f, 0.55f, 1.0f) : ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s", label);
    ImGui::SameLine();
    ImGui::Text("%s", value);
}

void UI::Render() {
    // Apply modern style on first render
    static bool styleInitialized = false;
    if (!styleInitialized) {
        SetupModernStyle();
        styleInitialized = true;
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.96f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (g_showMenu) {
        ImGui::SetNextWindowSize(ImVec2(580, 380), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(450, 300), ImVec2(1200, 800));
        
        if (ImGui::Begin("##GoWCoOpMain", &g_showMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
            // Custom title bar
            ImGui::PushFont(nullptr); // Use default font
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "GoW Co-Op v3");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "| Simple Co-op");
            ImGui::Separator();
            ImGui::Spacing();
            
            auto& es = GameState::GetSystem();
            auto& local = es.GetLocal();
            auto& guest = es.GetGuest();

            // Entity Status Section
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.11f, 0.5f));
            if (ImGui::BeginChild("##EntityStatus", ImVec2(0, 140), true)) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "ENTITY STATUS");
                ImGui::Separator();
                ImGui::Spacing();
                
                // Local player
                ImGui::PushStyleColor(ImGuiCol_Text, local.ptr ? ImVec4(0.4f, 0.8f, 0.6f, 1.0f) : ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
                ImGui::BulletText("Local Player: %s", local.ptr ? "ACTIVE" : "NOT FOUND");
                ImGui::PopStyleColor();
                
                if (local.ptr) {
                    ImGui::Indent(20);
                    ImGui::Text("Position: (%.2f, %.2f, %.2f)", local.pos.x, local.pos.y, local.pos.z);
                    ImGui::Text("Pointer: 0x%llX", (unsigned long long)local.ptr);
                    ImGui::Unindent(20);
                }
                
                ImGui::Spacing();
                
                // Guest player
                ImGui::PushStyleColor(ImGuiCol_Text, guest.ptr ? ImVec4(0.4f, 0.8f, 0.6f, 1.0f) : ImVec4(0.6f, 0.6f, 0.7f, 1.0f));
                ImGui::BulletText("Guest Player: %s", guest.ptr ? "ACTIVE" : "NONE");
                ImGui::PopStyleColor();
                
                if (guest.ptr) {
                    ImGui::Indent(20);
                    ImGui::Text("Position: (%.2f, %.2f, %.2f)", guest.pos.x, guest.pos.y, guest.pos.z);
                    ImGui::Text("Soft ID: 0x%llX", (unsigned long long)es.GetGuestSoftId());
                    ImGui::Unindent(20);
                } else {
                    ImGui::Indent(20);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "Waiting for guest entity...");
                    ImGui::Unindent(20);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            
            // Network Status Section
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.11f, 0.5f));
            if (ImGui::BeginChild("##NetworkStatus", ImVec2(0, 110), true)) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "NETWORK STATUS");
                ImGui::Separator();
                ImGui::Spacing();
                
                auto& nm = NetworkManager::Get();
                auto stats = nm.GetStats();
                
                // Role badge
                ImGui::Text("Role: ");
                ImGui::SameLine();
                if (nm.GetRole() == NetRole::Host) {
                    DrawStatusBadge("HOST", ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
                } else if (nm.GetRole() == NetRole::Client) {
                    DrawStatusBadge("CLIENT", ImVec4(0.4f, 0.7f, 0.9f, 1.0f));
                } else {
                    DrawStatusBadge("NONE", ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                }
                
                ImGui::SameLine();
                ImGui::Text("| State: ");
                ImGui::SameLine();
                if (nm.GetState() == NetState::Connected) {
                    DrawStatusBadge("CONNECTED", ImVec4(0.4f, 0.85f, 0.5f, 1.0f));
                } else if (nm.GetState() == NetState::Listening) {
                    DrawStatusBadge("LISTENING", ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
                } else {
                    DrawStatusBadge("DISCONNECTED", ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
                }
                
                ImGui::Spacing();
                
                // Stats row
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "Latency: ");
                ImGui::SameLine();
                if (stats.latencyMs < 50) {
                    DrawStatusBadge("%.0f ms (Excellent)", ImVec4(0.4f, 0.85f, 0.5f, 1.0f), stats.latencyMs);
                } else if (stats.latencyMs < 100) {
                    DrawStatusBadge("%.0f ms (Good)", ImVec4(0.9f, 0.8f, 0.3f, 1.0f), stats.latencyMs);
                } else {
                    DrawStatusBadge("%.0f ms (High)", ImVec4(0.9f, 0.4f, 0.4f, 1.0f), stats.latencyMs);
                }
                
                ImGui::SameLine(250);
                ImGui::Text("Packets: S=%u R=%u", stats.packetsSent, stats.packetsRecv);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            // Controls hint
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), 
                "Controls: F1 - Toggle Menu | F8 - Network Window");
        }
        ImGui::End();
    }

    // Network Window - Redesigned
    if (g_showNet) {
        ImGui::SetNextWindowSize(ImVec2(480, 280), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("##NetworkPanel", &g_showNet, ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "NETWORK CONTROL");
            ImGui::Separator();
            ImGui::Spacing();
            
            static char ipBuf[64] = "127.0.0.1";
            static int port = 7777;
            static bool hostMode = false;
            
            // Mode selection
            ImGui::PushStyleColor(ImGuiCol_Button, hostMode ? ImVec4(0.3f, 0.2f, 0.1f, 1.0f) : ImVec4(0.18f, 0.18f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hostMode ? ImVec4(0.4f, 0.3f, 0.2f, 1.0f) : ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
            if (ImGui::Button("Host Mode", ImVec2(100, 30))) {
                hostMode = true;
            }
            ImGui::PopStyleColor(2);
            
            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, !hostMode ? ImVec4(0.15f, 0.2f, 0.3f, 1.0f) : ImVec4(0.18f, 0.18f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, !hostMode ? ImVec4(0.2f, 0.3f, 0.4f, 1.0f) : ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
            if (ImGui::Button("Client Mode", ImVec2(100, 30))) {
                hostMode = false;
            }
            ImGui::PopStyleColor(2);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Connection settings
            ImGui::Text("Server IP:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##IP", ipBuf, IM_ARRAYSIZE(ipBuf));
            
            ImGui::Text("Port:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##Port", &port);
            
            ImGui::Spacing();
            
            // Action buttons
            float btnWidth = (ImGui::GetContentRegionAvail().x - 10) / 2;
            
            if (hostMode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.35f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.3f, 1.0f));
                if (ImGui::Button("Start Host Server", ImVec2(btnWidth, 35))) {
                    NetworkManager::Get().StartHost(port);
                }
                ImGui::PopStyleColor(2);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.5f, 1.0f));
                if (ImGui::Button("Connect to Server", ImVec2(btnWidth, 35))) {
                    NetworkManager::Get().Connect(ipBuf, port);
                }
                ImGui::PopStyleColor(2);
            }
            
            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Disconnect", ImVec2(btnWidth, 35))) {
                NetworkManager::Get().Disconnect();
            }
            ImGui::PopStyleColor(2);
            
            ImGui::Spacing();
            ImGui::Separator();
            
            // Current status
            auto& nm = NetworkManager::Get();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "Current Status:");
            ImGui::SameLine();
            
            const char* roleStr = nm.GetRole() == NetRole::Host ? "HOST" :
                                  nm.GetRole() == NetRole::Client ? "CLIENT" : "NONE";
            const char* stateStr = nm.GetState() == NetState::Connected ? "CONNECTED" :
                                   nm.GetState() == NetState::Listening ? "LISTENING" : "DISCONNECTED";
            
            ImGui::Text("%s | %s", roleStr, stateStr);
        }
        ImGui::End();
    }

    ImGui::PopStyleVar(2);
}
