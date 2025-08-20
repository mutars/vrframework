#pragma once

#include <engine/memory/offsets.h>
#include <safetyhook/inline_hook.hpp>
#include <utility/PointerHook.hpp>
#include <utils/FunctionHook.h>

class EngineRendererModule
{
public:
    void InstallHooks();

    inline static EngineRendererModule* Get()
    {
        static auto instance(new EngineRendererModule);
        return instance;
    }
private:
    EngineRendererModule()  = default;
    ~EngineRendererModule() = default;

    safetyhook::InlineHook m_worldTickHook{};
    safetyhook::InlineHook m_renderSubmitHook{};
    safetyhook::InlineHook m_checkResolutionHook{};

    static uintptr_t onWorldTick();
    static bool      checkResolution(int *outFlags, int maxMessagesToProcess, char filterSpecialMessages);
};
