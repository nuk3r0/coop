#include "network_manager.h"
#include "../utils/logger.h"
#include "../game/game_state.h"
#include "../resolver/resolver.h"
#include "../memory/memory.h"
#include "../config/gow_config.h"
#include <cstring>

using namespace std::chrono;

bool NetworkManager::Initialize() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
    m_running = true;
    m_recvThread = std::thread(&NetworkManager::ReceiveThread, this);
    LOG_INFO("NetworkManager initialized");
    return true;
}

void NetworkManager::Shutdown() {
    m_running = false;
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_recvThread.joinable()) m_recvThread.join();
    WSACleanup();
    LOG_INFO("NetworkManager shutdown");
}

bool NetworkManager::StartHost(int port) {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    u_long nonblock = 1;
    ioctlsocket(m_socket, FIONBIO, &nonblock);

    m_role = NetRole::Host;
    m_state = NetState::Listening;
    ClearPeer();
    LOG_INFO("Host listening on port %d", port);
    return true;
}

bool NetworkManager::Connect(const std::string& ip, int port) {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) return false;

    u_long nonblock = 1;
    ioctlsocket(m_socket, FIONBIO, &nonblock);

    sockaddr_in peer{};
    peer.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &peer.sin_addr);
    peer.sin_port = htons(port);
    SetPeer(peer);

    m_role = NetRole::Client;
    m_state = NetState::Connecting;
    m_targetIp = ip;
    m_targetPort = port;

    ConnectionRequestPacket req{};
    req.header.type = PacketType::ConnectionRequest;
    req.header.sequence = ++m_sequence;
    req.header.timestamp = static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());

    SendPacket(req.header, &req.padding, sizeof(req) - sizeof(PacketHeader), nullptr);

    LOG_INFO("Connecting to %s:%d", ip.c_str(), port);
    return true;
}

void NetworkManager::Disconnect() {
    m_state = NetState::Disconnected;
    ClearPeer();
    m_role = NetRole::None;
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    LOG_INFO("Disconnected");
}

void NetworkManager::ReceiveThread() {
    char buf[4096];
    sockaddr_in from{};
    int fromLen = sizeof(from);
    while (m_running) {
        if (m_socket == INVALID_SOCKET) {
            std::this_thread::sleep_for(10ms);
            continue;
        }
        int len = recvfrom(m_socket, buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);
        if (len > 0) {
            m_lastRecvTimeMs.store(
                duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count(),
                std::memory_order_relaxed
            );
            ProcessPacket(from, buf, len);
            RecordPacketReceived();
        } else if (len == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                // handle real error if needed
            }
            std::this_thread::sleep_for(1ms);
        }
    }
}

void NetworkManager::ProcessPacket(const sockaddr_in& from, const char* data, int len) {
    if (len < static_cast<int>(sizeof(PacketHeader))) return;
    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(data);
    if (hdr->magic != 0x434F5747) return;

    if (m_role == NetRole::Host) {
        if (!HasPeer()) {
            SetPeer(from);
            m_state = NetState::Connected;
            LOG_INFO("Client connected from %s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));

            // Pick guest entity and send its soft_id
            uintptr_t local = GameState::GetLocalPtr();
            uintptr_t guest = local ? Resolver::PickGuestEntity(local) : 0;
            uint64_t guestSoftId = guest ? Resolver::CalcSoftId(guest) : 0;
            if (guest) {
                GameState::SetGuestPtr(guest);
                GameState::SetGuestSoftId(guestSoftId);
                LOG_INFO("[Net] Guest assigned: 0x%llX name='%s'",
                         (unsigned long long)guest, Resolver::ReadName(guest).c_str());
            }

            ConnectionAcceptedPacket ack{};
            ack.header.type = PacketType::ConnectionAccepted;
            ack.header.sequence = ++m_sequence;
            ack.guestSoftId = guestSoftId;
            SendPacket(ack.header, &ack.clientId, sizeof(ack) - sizeof(PacketHeader), nullptr);
        }
        // Client sends PlayerSnapshot — write to guest entity
        if (hdr->type == PacketType::PlayerSnapshot && len >= static_cast<int>(sizeof(PlayerSnapshot))) {
            const auto* snap = reinterpret_cast<const PlayerSnapshot*>(data);
            uintptr_t guest = GameState::GetSystem().GetGuest().ptr;
            if (guest) {
                Memory::Write<float>(guest + g_gow_config.pos_x, snap->pos.x);
                Memory::Write<float>(guest + g_gow_config.pos_y, snap->pos.y);
                Memory::Write<float>(guest + g_gow_config.pos_z, snap->pos.z);
            }
        }
    } else if (m_role == NetRole::Client) {
        if (hdr->type == PacketType::ConnectionAccepted) {
            const auto* ack = reinterpret_cast<const ConnectionAcceptedPacket*>(data);
            m_state = NetState::Connected;
            uint64_t guestSoftId = ack->guestSoftId;
            LOG_INFO("Connected to host. Guest soft_id: 0x%llX",
                     (unsigned long long)guestSoftId);

            // Find guest entity in local g_list
            if (guestSoftId) {
                uintptr_t guest = Resolver::FindEntityBySoftId(guestSoftId);
                if (guest) {
                    GameState::SetGuestPtr(guest);
                    GameState::SetGuestSoftId(guestSoftId);
                    LOG_INFO("[Net] Found guest entity @ 0x%llX name='%s'",
                             (unsigned long long)guest, Resolver::ReadName(guest).c_str());
                }
            }
        } else if (hdr->type == PacketType::WorldState && len >= static_cast<int>(sizeof(WorldStatePacket))) {
            const auto* pkt = reinterpret_cast<const WorldStatePacket*>(data);

            // Write host's position to guest entity (client sees host as guest character)
            uintptr_t guest = GameState::GetSystem().GetGuest().ptr;

            // If guest not set yet, try to find by soft_id from WorldStatePacket
            if (!guest && pkt->guestSoftId) {
                guest = Resolver::FindEntityBySoftId(pkt->guestSoftId);
                if (guest) {
                    GameState::SetGuestPtr(guest);
                    GameState::SetGuestSoftId(pkt->guestSoftId);
                    LOG_INFO("[Net] Guest found via WorldState: 0x%llX", (unsigned long long)guest);
                }
            }

            if (guest) {
                // Write host's position to guest entity on client's screen
                Memory::Write<float>(guest + g_gow_config.pos_x, pkt->hostPlayer.pos.x);
                Memory::Write<float>(guest + g_gow_config.pos_y, pkt->hostPlayer.pos.y);
                Memory::Write<float>(guest + g_gow_config.pos_z, pkt->hostPlayer.pos.z);
            }
        }
    }
}

void NetworkManager::SendPacket(const PacketHeader& header, const void* data, size_t size, const sockaddr_in* addr) {
    if (m_socket == INVALID_SOCKET) return;
    char buf[4096];
    std::memcpy(buf, &header, sizeof(header));
    if (data && size > 0) std::memcpy(buf + sizeof(header), data, size);

    const sockaddr_in* dest = addr;
    sockaddr_in peerCopy;
    if (!dest) {
        if (GetPeer(peerCopy)) {
            dest = &peerCopy;
        } else {
            return;
        }
    }

    sendto(m_socket, buf, static_cast<int>(sizeof(header) + size), 0, (sockaddr*)dest, sizeof(*dest));
    RecordPacketSent();
}

void NetworkManager::SendInput(const PlayerInput& input) {
    if (m_role != NetRole::Client || m_state != NetState::Connected) return;
    InputPacket p{};
    p.header.type = PacketType::Input;
    p.header.sequence = ++m_sequence;
    p.header.timestamp = static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    p.input = input;
    SendPacket(p.header, &p.input, sizeof(p) - sizeof(PacketHeader), nullptr);
}

void NetworkManager::SendWorldState(const WorldStatePacket& state) {
    if (m_role != NetRole::Host || m_state != NetState::Connected) return;
    WorldStatePacket s = state;
    s.header.type = PacketType::WorldState;
    SendPacket(s.header, reinterpret_cast<const char*>(&s) + sizeof(PacketHeader), sizeof(s) - sizeof(PacketHeader), nullptr);
}

void NetworkManager::SendEvent(const EventPacket& ev) {
    SendPacket(ev.header, reinterpret_cast<const char*>(&ev) + sizeof(PacketHeader), sizeof(ev) - sizeof(PacketHeader), nullptr);
}

void NetworkManager::SendPlayerSnapshot(PlayerSnapshot snap) {
    if (m_role != NetRole::Client || m_state != NetState::Connected) return;
    snap.header.type = PacketType::PlayerSnapshot;
    snap.header.sequence = ++m_sequence;
    snap.header.timestamp = static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    SendPacket(snap.header, &snap.pos, sizeof(snap) - sizeof(PacketHeader), nullptr);
}

void NetworkManager::Update() {
    if (m_role == NetRole::Client && m_state == NetState::Connected) {
        auto nowMs = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        auto last = m_lastRecvTimeMs.load(std::memory_order_relaxed);
        if ((nowMs - last) > 5000) {
            LOG_WARNING("Connection lost, reconnecting...");
            m_state = NetState::Reconnecting;
        }
    }
    if (m_role == NetRole::Client && m_state == NetState::Reconnecting) {
        static auto lastTry = steady_clock::now();
        auto now = steady_clock::now();
        if (duration_cast<seconds>(now - lastTry).count() >= 3) {
            lastTry = now;
            Connect(m_targetIp, m_targetPort);
        }
    }
}

// --- Thread-safe peer / stats implementations ---

void NetworkManager::SetPeer(const sockaddr_in& addr) {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    m_peerAddr = addr;
    m_hasPeer = true;
}

void NetworkManager::ClearPeer() {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    m_hasPeer = false;
}

bool NetworkManager::HasPeer() const {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    return m_hasPeer;
}

bool NetworkManager::GetPeer(sockaddr_in& out) const {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    if (!m_hasPeer) return false;
    out = m_peerAddr;
    return true;
}

NetworkStats NetworkManager::GetStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}
