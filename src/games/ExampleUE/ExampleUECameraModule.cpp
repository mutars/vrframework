#include "ExampleUECameraModule.h"
#include "memory/offsets.h"
#include "aer/ConstantsPool.h"
#include <mods/VR.hpp>
#include <spdlog/spdlog.h>

ExampleUECameraModule* ExampleUECameraModule::get() {
    static auto inst = new ExampleUECameraModule();
    return inst;
}

void ExampleUECameraModule::installHooks() {
    spdlog::info("ExampleUECameraModule::installHooks - Installing camera hooks");
    
    auto calcViewFn = memory::calcViewAddr();
    if (calcViewFn) {
        m_calcViewHook = safetyhook::create_inline(reinterpret_cast<void*>(calcViewFn), 
                                                    reinterpret_cast<void*>(&onCalcView));
        spdlog::info("ExampleUECameraModule: CalcView hook installed");
    } else {
        spdlog::warn("ExampleUECameraModule: CalcView address not found");
    }
    
    auto getProjectionFn = memory::getProjectionAddr();
    if (getProjectionFn) {
        m_getProjectionHook = safetyhook::create_inline(reinterpret_cast<void*>(getProjectionFn), 
                                                         reinterpret_cast<void*>(&onGetProjection));
        spdlog::info("ExampleUECameraModule: GetProjection hook installed");
    } else {
        spdlog::warn("ExampleUECameraModule: GetProjection address not found");
    }
}

void ExampleUECameraModule::onCalcView(sdk::APlayerCameraManager* camMgr, float dt, 
                                        sdk::FMinimalViewInfo* outView) {
    auto inst = get();
    inst->m_calcViewHook.call<void>(camMgr, dt, outView);
    
    auto vr = VR::get();
    if (vr->is_hmd_active()) {
        // Get VR transforms
        auto eye = vr->get_current_eye_transform();
        auto hmd = vr->get_transform(0);
        auto offset = vr->get_transform_offset();
        
        // Apply VR transform to outView->Location/Rotation
        // Note: This is a simplified example. In a real implementation,
        // you would properly transform the camera location and rotation
        // based on the HMD pose and eye offset.
        
        spdlog::debug("ExampleUECameraModule: Applied VR transforms to camera view");
    }
}

void ExampleUECameraModule::onGetProjection(glm::mat4* outProj, float fov, float aspect, 
                                             float nearZ, float farZ) {
    auto inst = get();
    inst->m_getProjectionHook.call<void>(outProj, fov, aspect, nearZ, farZ);
    
    auto vr = VR::get();
    if (vr->is_hmd_active()) {
        // Submit projection matrix to global pool for VR rendering
        GlobalPool::submit_projection(*outProj, vr->m_render_frame_count);
        spdlog::debug("ExampleUECameraModule: Submitted projection matrix to pool");
    }
}
