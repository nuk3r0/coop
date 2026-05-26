#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include "../network/packet.h"

namespace Resolver {

    // Resolve local player via CE static pointer
    uintptr_t ResolveLocalPlayer();

    // Traverse g_list and return all entity pointers
    std::vector<uintptr_t> TraverseGList();

    // Identity helpers
    uint64_t CalcSoftId(uintptr_t entity_ptr);
    std::string ReadName(uintptr_t entity_ptr);
    Vec3 ReadPos(uintptr_t entity_ptr);

    // Guest selection: first non-self entity with finite pos
    uintptr_t PickGuestEntity(uintptr_t local_ptr);

    // Find entity in g_list by soft_id
    uintptr_t FindEntityBySoftId(uint64_t soft_id);

}
