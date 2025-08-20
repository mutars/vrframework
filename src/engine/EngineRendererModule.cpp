//
// Created by sergp on 6/26/2024.
//

#include "EngineRendererModule.h"
#include "EngineEntry.h"
#include "REL/Relocation.h"
#include "engine/memory/offsets.h"
#include <engine/models/ModSettings.h>
#include <mods/VR.hpp>
#include <safetyhook/easy.hpp>

std::vector<uint8_t> GenerateAspectRatioPatchBytes() {
    // --- The Assembly We Want ---
    // movaps xmm6, xmm3   ; Move the correct aspect ratio (v43 in xmm3)
    //                     ; into the target aspect ratio register (v24 in xmm6).
    //
    // --- Bytecode for this instruction ---
    // 0F 28 F3

    std::vector<uint8_t> patchBytes = { 0x0F, 0x28, 0xF3 };

    // --- Calculate Padding ---
    // The original code block we are replacing starts at RVA 0x991695
    // and ends just before RVA 0x9916CB.
    // Total size = 0x9916CB - 0x991695 = 0x36 bytes (54 bytes).
    // Our patch is 3 bytes long.
    // We need to fill the remaining space with NOPs.
    const size_t originalCodeSize = 54;
    const size_t patchSize = patchBytes.size();
    const size_t nopCount = originalCodeSize - patchSize;

    // A NOP (No-Operation) instruction is 0x90 in x86-64.
    for (size_t i = 0; i < nopCount; ++i) {
        patchBytes.push_back(0x90);
    }

    return patchBytes;
}

bool PatchMemory(uintptr_t address, const std::vector<uint8_t>& patchBytes) {
    if (patchBytes.empty()) {
        return false;
    }

    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)address, patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy((LPVOID)address, patchBytes.data(), patchBytes.size());
    DWORD tempProtect;
    VirtualProtect((LPVOID)address, patchBytes.size(), oldProtect, &tempProtect);

    return true;
}

void DisableLetterBox() {
    //TODO move to memory
    uintptr_t patchAddress = (uintptr_t)memory::mod + 0x991695;

    // Generate the bytecode for our patch.
    std::vector<uint8_t> patch = GenerateAspectRatioPatchBytes();

    spdlog::info("Target address: 0x{:x}", patchAddress);
    spdlog::info("Patch size: {} bytes.", patch.size());

    // Apply the patch to memory.
    if (PatchMemory(patchAddress, patch)) {
        spdlog::info("Success! Aspect ratio clamp has been patched.");
    } else {
        spdlog::error("Failed to write patch to memory. GetLastError() = {}", GetLastError());
    }
}

void EngineRendererModule::InstallHooks()
{
    spdlog::info("Installing EngineRendererModule hooks");
    DisableLetterBox();

    auto worldTickFunct = (uintptr_t) memory::mod + 0x406820;
    m_worldTickHook = safetyhook::create_inline((void*)worldTickFunct, (void*)&EngineRendererModule::onWorldTick);
    if(!m_worldTickHook) {
        spdlog::error("Failed to hook world tick");
    }

    auto checkResolutionFunct = (uintptr_t) memory::mod + 0x98fbe0;
    m_checkResolutionHook = safetyhook::create_inline((void*)checkResolutionFunct, (void*)&EngineRendererModule::checkResolution);
    if(!m_checkResolutionHook) {
        spdlog::error("Failed to hook check resolution");
    }
}

uintptr_t EngineRendererModule::onWorldTick() {
    static auto instance = Get();
    static     auto        vr            = VR::get();
    g_framework->run_imgui_frame(false);
    vr->on_engine_tick(nullptr);
    vr->on_begin_rendering(nullptr);
    auto result = instance->m_worldTickHook.call<uintptr_t>();
    return result;
}

bool EngineRendererModule::checkResolution(int* outFlags, int maxMessagesToProcess, char filterSpecialMessages)
{
    static auto instance = Get();
    static auto vr = VR::get();
    //TODO move to offsets
    static auto pRenderW = (int*)((uintptr_t)memory::mod + 0x504c398);
    static auto pRenderH = (int*)((uintptr_t)memory::mod + 0x504c39c);
    int oldRenderW = *pRenderW;
    int oldRenderH = *pRenderH;
    auto resp = instance->m_checkResolutionHook.call<bool>(outFlags, maxMessagesToProcess, filterSpecialMessages);
    auto vrWidth = vr->get_hmd_width();
    auto vrHeight = vr->get_hmd_height();
    if (vr->is_hmd_active() && oldRenderW > 0 && oldRenderH > 0 && vrWidth > 0 && vrHeight > 0) {
        if(oldRenderW == vrWidth && oldRenderH == vrHeight) {
            *pRenderW = vrWidth;
            *pRenderH = vrHeight;
            *outFlags ^= 0x1;
        } else if(*pRenderW != vrWidth || *pRenderH != vrHeight) {
            spdlog::info("Changing render size to {}x{}", vrWidth, vrHeight);
            *pRenderW = vrWidth;
            *pRenderH = vrHeight;
            *outFlags |= 0x1;
        }
    }
    return resp;
}
