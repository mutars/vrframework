#include "ExampleUECameraModule.h"
#include "memory/offsets.h"
#include "aer/ConstantsPool.h"
#include "mods/VR.hpp"

void ExampleUECameraModule::installHooks() {
    auto calcViewFn = memory::calcViewAddr();
    if (calcViewFn != 0) {
        m_calcViewHook = safetyhook::create_inline(
            reinterpret_cast<void*>(calcViewFn), 
            reinterpret_cast<void*>(&onCalcView)
        );
    }
    
    auto getProjectionFn = memory::getProjectionAddr();
    if (getProjectionFn != 0) {
        m_getProjectionHook = safetyhook::create_inline(
            reinterpret_cast<void*>(getProjectionFn), 
            reinterpret_cast<void*>(&onGetProjection)
        );
    }
}

void ExampleUECameraModule::onCalcView(sdk::APlayerCameraManager* camMgr, float dt, sdk::FMinimalViewInfo* outView) {
    auto inst = get();
    inst->m_calcViewHook.call<void>(camMgr, dt, outView);
    
    auto vr = VR::get();
    if (vr->is_hmd_active() && outView != nullptr) {
        // Get VR transforms for applying to camera view
        auto eye = vr->get_current_eye_transform();
        auto hmd = vr->get_transform(0);
        auto offset = vr->get_transform_offset();
        
        // TODO: Apply VR transform to outView->Location/Rotation
        // This requires game-specific SDK structure knowledge:
        // - Extract position from hmd matrix
        // - Extract rotation from eye matrix
        // - Apply offset compensation
        // - Write to outView->Location and outView->Rotation
        //
        // Example implementation (requires actual SDK structures):
        // glm::vec3 hmdPos = glm::vec3(hmd[3]);
        // outView->Location = FVector(hmdPos.x, hmdPos.y, hmdPos.z);
        (void)eye;
        (void)hmd;
        (void)offset;
    }
}

void ExampleUECameraModule::onGetProjection(glm::mat4* outProj, float fov, float aspect, float nearZ, float farZ) {
    auto inst = get();
    inst->m_getProjectionHook.call<void>(outProj, fov, aspect, nearZ, farZ);
    
    auto vr = VR::get();
    if (vr->is_hmd_active() && outProj != nullptr) {
        GlobalPool::submit_projection(*outProj, vr->m_render_frame_count);
    }
}
