#pragma once

#include <engine/memory/offsets.h>
#include <safetyhook/inline_hook.hpp>
#include <utility/PointerHook.hpp>
#include <utils/FunctionHook.h>

namespace sdk {
    struct GameObject;
}

class UIComponent
{
public:
    void InstallHooks();

    inline static UIComponent* Get()
    {
        static auto instance(new UIComponent);
        return instance;
    }

private:
    UIComponent()  = default;
    ~UIComponent() = default;

    safetyhook::InlineHook m_onSetGOScreenEdge{};
    static uintptr_t onSetGOScreenEdge(sdk::GameObject* a1);
    
    void ApplyCircularBoundary(sdk::GameObject* object);
    static float CalculateOptimalRadius();
};
