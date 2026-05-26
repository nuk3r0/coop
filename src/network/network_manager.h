#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <string>
#include <chrono>
#include "packet.h"

#pragma comment(lib, "ws2_32.lib")

enum class NetRole { None, Host, Client };
enum class NetState { Disconnected, Listening, Connecting, Connected, Reconnecting };

struct NetworkStats {
    float latencyMs = 0.0f;
    uint32_t packetsSent = 0;
    uint32_t packetsRecv = 0;
};

class NetworkManager {
public:
    using StateCallback = std::function<void(const WorldStatePacket&)>;
    using InputCallback = std::function<void(const InputPacket&)>;
    using SnapshotCallback = std::function<void(const PlayerSnapshot&)>;

    static NetworkManager& Get() {
        static NetworkManager nm;
        return nm;
    }

    bool Initialize();
    void Shutdown();

    bool StartHost(int port);
    bool Connect(const std::string& ip, int port);
    void Disconnect();

    void Update(); // call every frame/tick

    void SendInput(const PlayerInput& input);
    void SendWorldState(const WorldStatePacket& state);
    void SendEvent(const EventPacket& ev);
    void SendPlayerSnapshot(PlayerSnapshot snap);

    NetRole GetRole() const { return m_role; }
    NetState GetState() const { return m_state; }
    
    // Returns a snapshot of stats (thread-safe copy)
    NetworkStats GetStats() const;

    void SetStateCallback(StateCallback cb) { m_stateCb = cb; }
    void SetInputCallback(InputCallback cb) { m_inputCb = cb; }
    void SetSnapshotCallback(SnapshotCallback cb) { m_snapshotCb = cb; }

    // Peer access (thread-safe)
    void SetPeer(const sockaddr_in& addr);
    void ClearPeer();
    bool HasPeer() const;
    bool GetPeer(sockaddr_in& out) const;

    // Stats helpers (thread-safe)
    void RecordPacketSent() { std::lock_guard<std::mutex> lock(m_statsMutex); ++m_stats.packetsSent; }
    void RecordPacketReceived() { std::lock_guard<std::mutex> lock(m_statsMutex); ++m_stats.packetsRecv; }
    void SetLatency(float ms) { std::lock_guard<std::mutex> lock(m_statsMutex); m_stats.latencyMs = ms; }

private:
    void ReceiveThread();
    void ProcessPacket(const sockaddr_in& from, const char* data, int len);
    void SendPacket(const PacketHeader& header, const void* data, size_t size, const sockaddr_in* addr = nullptr);

    SOCKET m_socket = INVALID_SOCKET;
    std::atomic<NetRole> m_role{NetRole::None};
    std::atomic<NetState> m_state{NetState::Disconnected};
    std::thread m_recvThread;
    std::atomic<bool> m_running{false};

    // Peer state (protected)
    mutable std::mutex m_peerMutex;
    sockaddr_in m_peerAddr{};
    bool m_hasPeer = false;

    // Stats (protected)
    mutable std::mutex m_statsMutex;
    NetworkStats m_stats;

    uint32_t m_sequence = 0;

    StateCallback m_stateCb;
    InputCallback m_inputCb;
    SnapshotCallback m_snapshotCb;

    std::string m_targetIp;
    int m_targetPort = 0;
    std::atomic<uint64_t> m_lastRecvTimeMs{0};
};
