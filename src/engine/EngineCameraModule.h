#pragma once

#include <glm/detail/type_mat4x4.hpp>
#include <safetyhook/inline_hook.hpp>
#include <utils/FunctionHook.h>

namespace sdk {
    namespace camera {
        struct Client;
    }
    struct Matrix4x4 {
        glm::vec4 row[4];
    };
    struct Viewport
    {
        char pad[236];
        float perspectiveFov;
        char padF0[228];
        float orpfoFov;
        char pad1D8[271];
        uint8_t orpho;
    };

    struct CameraObj
    {
        void *__vtbl;
        char pad8[48];
        char *name;
        char pad40[18];
        uint16_t flags;
        char pad58[300];
        sdk::Viewport *pViewport;
        uint8_t flag;
        char pad190[23];
        float perspectiveFov;
        float orphoFov;
    };

    struct RendererContext
    {
        char pad[16];
        Matrix4x4 inverseModelMatrix;
        Matrix4x4 normalMVPMatrix;
        Matrix4x4 specializedMVPMatrix;
        Matrix4x4 viewProjection;
        Matrix4x4 finalMVPMatrix;
        Matrix4x4 modelMatrix;
        Matrix4x4 invViewProjection;
        Matrix4x4 viewportMatrix;
        Matrix4x4 viewOrOffset;
        char pad250[64];
        glm::vec4 depthParam0;
        glm::vec4 depthParam1;
        glm::vec4 depthParam2;
        float tanHalfFovY;
        float tanHalfFovX;
        float nearZ;
        float farZ;
        glm::vec4 viewportRect;
        float fovYscaleFactor;
        float fovXScaleFactor;
        uint8_t flag;
        float unkF_2EC;
        char pad2E9[16];
        CameraObj *cameraObjectPtr;
        uint8_t flag2;
        uintptr_t a3;
        uintptr_t pad318;
        int a2;
    };

    static_assert(offsetof(RendererContext, viewProjection) == 0xD0, "RendererContext::viewProjection offset");
    static_assert(offsetof(RendererContext, invViewProjection) == 0x190, "RendererContext::invViewProjection offset");
    static_assert(offsetof(RendererContext, unkF_2EC) == 0x2EC, "RendererContext::unkF_2EC offset");
}

class EngineCameraModule
{
public:
    void InstallHooks();

    inline static EngineCameraModule* Get()
    {
        static auto instance(new EngineCameraModule);
        return instance;
    }

private:
    EngineCameraModule()  = default;
    ~EngineCameraModule() = default;

    /**
     * Conclusion: The hypothesis is confirmed. The code implements a view matrix calculation for a Y-Up, right-handed system with a Yaw-Pitch-Roll (Y-X-Z) rotation order.
     */
    safetyhook::InlineHook m_onCalcProjection{};
    safetyhook::InlineHook m_onCalcFinalView{};
    safetyhook::InlineHook m_onGetOrphoFov{};
public:
    static sdk::Matrix4x4* onCalcProjection(sdk::RendererContext* a1, float scalingFactor, uint8_t flag, __int64 a4, float nearZ, float farZ, float fovYScaleFactor, float fovXScaleFactor,
                                               sdk::Matrix4x4* viewProjOut, sdk::Matrix4x4* invViewProjOut, float* outTanHalfFovY, float* outTanHalfFovX,
                                               sdk::Matrix4x4* viewMatrixOut);
    static void applyAdditionalOffsetForCameraViewFromBehaviourBlender(
                   __int64 pBehaviouralCameraBlender,
                   sdk::Matrix4x4 *viewMatrixIn,
                   float inFov,
                   sdk::Matrix4x4 *viewMatrixOut,
                   float *outFov,
                   float offset_scale);

    static float onGetOrphoFov(sdk::camera::Client* client);

};
