#pragma once
#include <cstdint>

namespace Replication {

    void Init();

    // Host: pick guest, send positions to client
    void HostTick();

    // Client: send position, receive host position
    void ClientTick();

}
