#include "ExampleUERendererModule.h"

#include "memory/offsets.h"
#include <Framework.hpp>
#include <mods/VR.hpp>

ExampleUERendererModule* ExampleUERendererModule::get() {
    static ExampleUERendererModule inst;
    return &inst;
}

void ExampleUERendererModule::installHooks() {
    auto beginFrameFn = memory::beginFrameAddr();
    if (beginFrameFn != 0) {
        m_beginFrameHook = safetyhook::create_inline(reinterpret_cast<void*>(beginFrameFn), reinterpret_cast<void*>(&onBeginFrame));
    }

    auto beginRenderFn = memory::beginRenderAddr();
    if (beginRenderFn != 0) {
        m_beginRenderHook = safetyhook::create_inline(reinterpret_cast<void*>(beginRenderFn), reinterpret_cast<void*>(&onBeginRender));
    }
}

uintptr_t ExampleUERendererModule::onBeginFrame() {
    auto inst = get();
    if (g_framework && g_framework->is_ready()) {
        auto vr = VR::get();
        vr->m_engine_frame_count++;
    }
    return inst->m_beginFrameHook.call<uintptr_t>();
}

uintptr_t ExampleUERendererModule::onBeginRender(void* ctx) {
    auto inst = get();
    if (g_framework && g_framework->is_ready()) {
        auto vr = VR::get();
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
