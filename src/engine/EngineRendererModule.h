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

    static char onCalcRenderSize(int* resolutions, float aspectRatio, uint8_t* recreateGraph, float* scale);

    static void setWindowSize();
//    uintptr_t onCopyConstantsBuffer(sdk::RendererConstantBuffer* pBuffer);

//    void onCopyResources(sdk::RenderPassData* pData);

private:
    EngineRendererModule()  = default;
    ~EngineRendererModule() = default;

    safetyhook::InlineHook m_simulationStartMarker{};
    safetyhook::InlineHook m_letterBoxHook{};
    std::unique_ptr<FunctionHook> m_slPCLSetMarker{};
    std::unique_ptr<PointerHook> m_slPCLSetMarker2{};
    static void onSimulationStartMarker();
};
