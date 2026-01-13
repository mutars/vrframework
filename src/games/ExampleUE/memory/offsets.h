#pragma once

#include "memory/memory_mul.h"
#include <cstdint>

namespace memory {

/**
 * Game-specific pattern/offset definitions for ExampleUE
 * 
 * IMPORTANT: These byte patterns are fragile and may break with game updates.
 * 
 * Recommended approaches for production use:
 * 1. Implement version detection to select patterns based on game version
 * 2. Use multiple fallback patterns for each function
 * 3. Consider using RTTI-based lookup when available (e.g., VTable method)
 * 4. Store static offsets for known game versions as fallback
 * 
 * Pattern format uses '?' for wildcard bytes that may vary between versions.
 * The FuncRelocation function will use signature scanning when SIGNATURE_SCAN
 * is defined, otherwise falls back to the static offset parameter.
 */

inline uintptr_t beginFrameAddr() {
    // Pattern for UE BeginFrame function
    // This pattern targets the function prologue which is more stable
    return FuncRelocation("BeginFrame", "48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 E8", 0x0);
}

inline uintptr_t beginRenderAddr() {
    // Pattern for UE BeginRender function
    // Uses standard x64 calling convention prologue
    return FuncRelocation("BeginRender", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30", 0x0);
}

inline uintptr_t calcViewAddr() {
    // Pattern for APlayerCameraManager::CalcView
    // Alternative: Use VTable("APlayerCameraManager", offset) if RTTI available
    return FuncRelocation("CalcView", "40 53 48 83 EC 40 48 8B DA 48 8B D1", 0x0);
}

inline uintptr_t getProjectionAddr() {
    // Pattern for projection matrix calculation
    // Look for SSE register saves as they're common in matrix functions
    return FuncRelocation("GetProjection", "48 83 EC 48 0F 29 74 24 ?", 0x0);
}

/**
 * Engine frame counter address (GFrameNumber or similar)
 * Reading directly from the engine's frame counter ensures proper synchronization
 * and eliminates frame lag issues compared to manually incrementing.
 * 
 * For UE4/UE5: Look for GFrameNumber global variable
 * Pattern typically references the frame counter increment location
 */
inline uintptr_t engineFrameCounterAddr() {
    // Pattern that references GFrameNumber - adjust for specific game
    // This is typically found near frame begin/end logic
    return InstructionRelocation("EngineFrameCounter", "8B 05 ? ? ? ? 89 05 ? ? ? ?", 2, 6, 0x0);
}

} // namespace memory
