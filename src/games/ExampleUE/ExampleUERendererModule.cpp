#include "ExampleUERendererModule.h"
#include "memory/offsets.h"
#include "mods/VR.hpp"
#include "Framework.hpp"
#include <atomic>

// Cached pointer to engine's frame counter (initialized during hook installation)
// Using atomic for thread-safe access
static std::atomic<int*> g_engineFrameCounterPtr{nullptr};

void ExampleUERendererModule::installHooks() {
    auto beginFrameFn = memory::beginFrameAddr();
    if (beginFrameFn != 0) {
        m_beginFrameHook = safetyhook::create_inline(
            reinterpret_cast<void*>(beginFrameFn), 
            reinterpret_cast<void*>(&onBeginFrame)
        );
    }
    
    auto beginRenderFn = memory::beginRenderAddr();
    if (beginRenderFn != 0) {
        m_beginRenderHook = safetyhook::create_inline(
            reinterpret_cast<void*>(beginRenderFn), 
            reinterpret_cast<void*>(&onBeginRender)
        );
    }
    
    // Cache pointer to engine's frame counter for direct access
    auto frameCounterAddr = memory::engineFrameCounterAddr();
    if (frameCounterAddr != 0) {
        g_engineFrameCounterPtr.store(reinterpret_cast<int*>(frameCounterAddr));
    }
}

uintptr_t ExampleUERendererModule::onBeginFrame() {
    auto inst = get();
    
    // Cache the engine frame count before the frame begins
    // This reads directly from the engine's GFrameNumber or equivalent
    int* framePtr = g_engineFrameCounterPtr.load();
    if (framePtr != nullptr) {
        // Validate pointer is within expected memory range before dereferencing
        if (reinterpret_cast<uintptr_t>(framePtr) > 0x10000) {
            inst->cacheEngineFrameCount(*framePtr);
        }
    }
    
    return inst->m_beginFrameHook.call<uintptr_t>();
}

uintptr_t ExampleUERendererModule::onBeginRender(void* ctx) {
    auto inst = get();
    
    // Call the original function first to let the engine update its frame state
    auto result = inst->m_beginRenderHook.call<uintptr_t>(ctx);
    
    if (g_framework->is_ready()) {
        auto vr = VR::get();
        
        // Use the engine's frame count directly - avoids sync issues
        int engineFrame = inst->getEngineFrameCount();
        
        g_framework->enable_engine_thread();
        g_framework->run_imgui_frame(false);
        vr->on_begin_rendering(engineFrame);
        vr->update_hmd_state(engineFrame);
    }
    
    return result;
}
