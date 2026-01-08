#pragma once

#include "memory/memory_mul.h"
#include <cstdint>

namespace memory {

// Game-specific pattern/offset definitions
// These patterns need to be updated for each game version

inline uintptr_t beginFrameAddr() {
    // Pattern for UE BeginFrame function
    // Example: "48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 E8"
    return FuncRelocation("BeginFrame", "48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 E8", 0x0);
}

inline uintptr_t beginRenderAddr() {
    // Pattern for UE BeginRender function
    // Example: "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30"
    return FuncRelocation("BeginRender", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30", 0x0);
}

inline uintptr_t calcViewAddr() {
    // Pattern for APlayerCameraManager::CalcView
    // Example: "40 53 48 83 EC 40 48 8B DA 48 8B D1"
    return FuncRelocation("CalcView", "40 53 48 83 EC 40 48 8B DA 48 8B D1", 0x0);
}

inline uintptr_t getProjectionAddr() {
    // Pattern for projection matrix calculation
    // Example: "48 83 EC 48 0F 29 74 24 ?"
    return FuncRelocation("GetProjection", "48 83 EC 48 0F 29 74 24 ?", 0x0);
}

} // namespace memory
