#include "ExampleUERendererModule.h"
#include "memory/offsets.h"
#include "sdk/ExampleUESDK.h"
#include <mods/VR.hpp>
#include <Framework.hpp>
#include <spdlog/spdlog.h>

// Initialize the global frame number pointer
namespace sdk {
    uint64_t* GFrameNumber = nullptr;
}

ExampleUERendererModule* ExampleUERendererModule::get() {
    static auto inst = new ExampleUERendererModule();
    return inst;
}

void ExampleUERendererModule::installHooks() {
    spdlog::info("ExampleUERendererModule::installHooks - Installing renderer hooks");
    
    // Locate GFrameNumber global variable in Unreal Engine
    // This allows us to read the engine's frame counter directly
    auto gframeNumberAddr = memory::getGFrameNumberAddr();
    if (gframeNumberAddr) {
        sdk::GFrameNumber = reinterpret_cast<uint64_t*>(gframeNumberAddr);
        spdlog::info("ExampleUERendererModule: GFrameNumber located at 0x{:X}", gframeNumberAddr);
    } else {
        spdlog::warn("ExampleUERendererModule: GFrameNumber address not found - frame sync may not be accurate");
    }
    
    auto beginFrameFn = memory::beginFrameAddr();
    if (beginFrameFn) {
        m_beginFrameHook = safetyhook::create_inline(reinterpret_cast<void*>(beginFrameFn), 
                                                      reinterpret_cast<void*>(&onBeginFrame));
        spdlog::info("ExampleUERendererModule: BeginFrame hook installed");
    } else {
        spdlog::warn("ExampleUERendererModule: BeginFrame address not found");
    }
    
    auto beginRenderFn = memory::beginRenderAddr();
    if (beginRenderFn) {
        m_beginRenderHook = safetyhook::create_inline(reinterpret_cast<void*>(beginRenderFn), 
                                                       reinterpret_cast<void*>(&onBeginRender));
        spdlog::info("ExampleUERendererModule: BeginRender hook installed");
    } else {
        spdlog::warn("ExampleUERendererModule: BeginRender address not found");
    }
}

uintptr_t ExampleUERendererModule::onBeginFrame() {
    auto inst = get();
    return inst->m_beginFrameHook.call<uintptr_t>();
}

uintptr_t ExampleUERendererModule::onBeginRender(void* ctx) {
    auto inst = get();
    
    if (g_framework->is_ready()) {
        auto vr = VR::get();
        
        // For Unreal Engine: Read frame number directly from the engine instead of manually incrementing
        // This ensures we're always in sync with the engine's actual frame count
        // and avoids issues with being one frame behind or out of sync
        if (sdk::GFrameNumber != nullptr) {
            // Use the engine's frame counter directly
            vr->m_engine_frame_count = static_cast<int>(*sdk::GFrameNumber);
        } else {
            // Fallback to manual increment if GFrameNumber wasn't found
            // This is the generic approach for unknown engines
            vr->m_engine_frame_count++;
            spdlog::warn_once("ExampleUERendererModule: Using fallback frame counting - consider locating GFrameNumber");
        }
        
        g_framework->enable_engine_thread();
        g_framework->run_imgui_frame(false);
        vr->m_render_frame_count = vr->m_engine_frame_count;
        vr->on_begin_rendering(vr->m_render_frame_count);
        vr->update_hmd_state(vr->m_render_frame_count);
        
        auto result = inst->m_beginRenderHook.call<uintptr_t>(ctx);
        
        vr->m_presenter_frame_count = vr->m_render_frame_count;
        return result;
    }
    
    return inst->m_beginRenderHook.call<uintptr_t>(ctx);
}
