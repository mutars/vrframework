#pragma once

#include <safetyhook/inline_hook.hpp>
#include <glm/glm.hpp>
#include <cstdint>

// Forward declarations for SDK types
namespace sdk {
    struct FMinimalViewInfo;
    struct APlayerCameraManager;
}

class ExampleUECameraModule {
public:
    static ExampleUECameraModule* get() {
        static auto inst = new ExampleUECameraModule();
        return inst;
    }
    
    void installHooks();

private:
    ExampleUECameraModule() = default;
    ~ExampleUECameraModule() = default;
    
    safetyhook::InlineHook m_calcViewHook{};
    safetyhook::InlineHook m_getProjectionHook{};
    
    static void onCalcView(sdk::APlayerCameraManager* camMgr, float dt, sdk::FMinimalViewInfo* outView);
    static void onGetProjection(glm::mat4* outProj, float fov, float aspect, float nearZ, float farZ);
};
