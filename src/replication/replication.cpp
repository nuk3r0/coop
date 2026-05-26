#include "replication.h"
#include "../network/network_manager.h"
#include "../network/packet.h"
#include "../memory/memory.h"
#include "../config/gow_config.h"
#include "../resolver/resolver.h"
#include "../game/game_state.h"
#include "../utils/logger.h"
#include <chrono>

namespace Replication {

    static uint32_t s_last_host_send_tick = 0;
    static uint32_t s_last_client_send_tick = 0;
    static constexpr int SEND_INTERVAL = 3; // every 3 ticks ~ 50ms

    void Init() {
        s_last_host_send_tick = 0;
        s_last_client_send_tick = 0;
    }

    void HostTick() {
        auto& nm = NetworkManager::Get();
        if (nm.GetRole() != NetRole::Host || nm.GetState() != NetState::Connected)
            return;

        // Rate-limit
        s_last_host_send_tick++;
        if (s_last_host_send_tick % SEND_INTERVAL != 0) return;

        // Ensure guest entity is assigned
        uintptr_t local = GameState::GetLocalPtr();
        if (!local) return;

        uintptr_t guest = GameState::GetSystem().GetGuest().ptr;
        uint64_t guestSoftId = GameState::GetGuestSoftId();

        if (!guest) {
            // Try to pick a guest
            guest = Resolver::PickGuestEntity(local);
            if (guest) {
                guestSoftId = Resolver::CalcSoftId(guest);
                GameState::SetGuestPtr(guest);
                GameState::SetGuestSoftId(guestSoftId);
                Vec3 gpos = Resolver::ReadPos(guest);
                LOG_INFO("[Replication] Guest assigned: ptr=0x%llX soft_id=0x%llX name='%s'",
                         (unsigned long long)guest, (unsigned long long)guestSoftId,
                         Resolver::ReadName(guest).c_str());
            }
        }

        if (!guest) return;

        Vec3 localPos = GameState::GetLocalPosition();
        Vec3 guestPos = GameState::GetSystem().GetGuest().pos;

        WorldStatePacket pkt{};
        pkt.header.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
        pkt.hostPlayer.pos = localPos;
        pkt.clientPlayer.pos = guestPos;
        pkt.guestSoftId = guestSoftId;

        nm.SendWorldState(pkt);
    }

    void ClientTick() {
        auto& nm = NetworkManager::Get();
        if (nm.GetRole() != NetRole::Client || nm.GetState() != NetState::Connected)
            return;

        // Rate-limit sends
        s_last_client_send_tick++;
        if (s_last_client_send_tick % SEND_INTERVAL != 0) return;

        Vec3 localPos = GameState::GetLocalPosition();

        PlayerSnapshot snap;
        snap.header.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
        snap.pos = localPos;

        nm.SendPlayerSnapshot(snap);
    }

}
