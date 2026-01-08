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

private:
    ExampleUERendererModule() = default;
    ~ExampleUERendererModule() = default;
    
    safetyhook::InlineHook m_beginFrameHook{};
    safetyhook::InlineHook m_beginRenderHook{};
    
    static uintptr_t onBeginFrame();
    static uintptr_t onBeginRender(void* context);
};
