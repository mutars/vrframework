//
// Created by sergp on 6/26/2024.
//

#include "EngineRendererModule.h"
#include "EngineEntry.h"
#include "engine/memory/memory_mul.h"
#include <mods/VR.hpp>
#include <safetyhook/easy.hpp>

void EngineRendererModule::InstallHooks()
{
    spdlog::info("Installing EngineRendererModule hooks");
    auto worldTickFunct = (uintptr_t) memory::g_mod + 0x406820;
    m_worldTickHook = safetyhook::create_inline((void*)worldTickFunct, (void*)&EngineRendererModule::onWorldTick);
    if(!m_worldTickHook) {
        spdlog::error("Failed to hook world tick");
    }

    auto checkResolutionFunct = (uintptr_t) memory::g_mod + 0x98fbe0;
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
    static auto pRenderW = (int*)((uintptr_t)memory::g_mod + 0x504c398);
    static auto pRenderH = (int*)((uintptr_t)memory::g_mod + 0x504c39c);
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
