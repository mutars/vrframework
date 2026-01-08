#pragma once
#include <safetyhook/inline_hook.hpp>

class ExampleUERendererModule {
public:
    static ExampleUERendererModule* get();
    void installHooks();

private:
    safetyhook::InlineHook m_beginFrameHook{};
    safetyhook::InlineHook m_beginRenderHook{};
    
    static uintptr_t onBeginFrame();
    static uintptr_t onBeginRender(void* context);
};
