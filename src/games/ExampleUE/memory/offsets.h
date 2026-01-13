#pragma once

#include "memory/memory_mul.h"

namespace memory {
    // Placeholder ExampleUE signatures; replace with game-specific patterns/offsets when integrating.
    inline uintptr_t beginFrameAddr() {
        return FuncRelocation("BeginFrame", "48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 E8", 0x0);
    }
    inline uintptr_t beginRenderAddr() {
        return FuncRelocation("BeginRender", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30", 0x0);
    }
    inline uintptr_t calcViewAddr() {
        return FuncRelocation("CalcView", "40 53 48 83 EC 40 48 8B DA 48 8B D1", 0x0);
    }
    inline uintptr_t getProjectionAddr() {
        return FuncRelocation("GetProjection", "48 83 EC 48 0F 29 74 24 ?", 0x0);
    }
} // namespace memory
