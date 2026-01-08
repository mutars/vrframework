#include "ExampleUERendererModule.h"
#include "memory/offsets.h"
#include <mods/VR.hpp>
#include <Framework.hpp>
#include <spdlog/spdlog.h>

ExampleUERendererModule* ExampleUERendererModule::get() {
    static auto inst = new ExampleUERendererModule();
    return inst;
}

void ExampleUERendererModule::installHooks() {
    spdlog::info("ExampleUERendererModule::installHooks - Installing renderer hooks");
    
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
        vr->m_engine_frame_count++;
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
