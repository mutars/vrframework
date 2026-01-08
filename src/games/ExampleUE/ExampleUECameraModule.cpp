#include "ExampleUECameraModule.h"

#include "memory/offsets.h"
#include "aer/ConstantsPool.h"
#include "sdk/ExampleUESDK.h"
#include <mods/VR.hpp>

ExampleUECameraModule* ExampleUECameraModule::get() {
    static ExampleUECameraModule inst;
    return &inst;
}

void ExampleUECameraModule::installHooks() {
    auto calcViewFn = memory::calcViewAddr();
    if (calcViewFn != 0) {
        m_calcViewHook = safetyhook::create_inline(reinterpret_cast<void*>(calcViewFn), reinterpret_cast<void*>(&onCalcView));
    }

    auto getProjectionFn = memory::getProjectionAddr();
    if (getProjectionFn != 0) {
        m_getProjectionHook = safetyhook::create_inline(reinterpret_cast<void*>(getProjectionFn), reinterpret_cast<void*>(&onGetProjection));
    }
}

void ExampleUECameraModule::onCalcView(sdk::APlayerCameraManager* camMgr, float dt, sdk::FMinimalViewInfo* outView) {
    auto inst = get();
    inst->m_calcViewHook.call<void>(camMgr, dt, outView);

    auto vr = VR::get();
    if (vr->is_hmd_active() && outView != nullptr) {
        const auto eye = vr->get_current_eye_transform();
        const auto hmd = vr->get_transform(0);
        const auto offset = vr->get_transform_offset();

        // Minimal application of HMD transform to the view info
        const auto view = hmd * offset * eye;
        outView->Location = glm::vec3{ view[3].x, view[3].y, view[3].z };
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
