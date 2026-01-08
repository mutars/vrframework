#pragma once

#include <safetyhook/inline_hook.hpp>
#include <cstdint>

class ExampleUERendererModule {
public:
    static ExampleUERendererModule* get() {
        static auto inst = new ExampleUERendererModule();
        return inst;
    }
    
    void installHooks();
    
    // Get the frame count directly from the engine
    // Override this to read from the game's frame counter (e.g., GFrameNumber)
    int getEngineFrameCount() const { return m_cachedEngineFrame; }
    void cacheEngineFrameCount(int frame) { m_cachedEngineFrame = frame; }

private:
    ExampleUERendererModule() = default;
    ~ExampleUERendererModule() = default;
    
    safetyhook::InlineHook m_beginFrameHook{};
    safetyhook::InlineHook m_beginRenderHook{};
    
    // Cached engine frame - should be set by reading from engine's frame counter
    int m_cachedEngineFrame{0};
    
    static uintptr_t onBeginFrame();
    static uintptr_t onBeginRender(void* context);
};
