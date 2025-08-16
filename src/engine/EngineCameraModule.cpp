#include "EngineCameraModule.h"
#include "REL/Relocation.h"
#include "engine/memory/offsets.h"
#include <engine/models/ModSettings.h>
#include <mods/VR.hpp>
#include <safetyhook/easy.hpp>

void EngineCameraModule::InstallHooks()
{
    spdlog::info("Installing EngineCameraModule hooks");

    REL::Relocation<void *> onCalcProjectionFN{ memory::calc_view_projection() };
    m_onCalcProjection = safetyhook::create_inline((void*)onCalcProjectionFN.address(), (void*)&EngineCameraModule::onCalcProjection);
    if(!m_onCalcProjection) {
        spdlog::error("Failed to hook m_onCalcProjection");
    }

    REL::Relocation<void *> calcFinalViewFn{ memory::calc_final_view_fn() };
    m_onCalcFinalView = safetyhook::create_inline((void*)calcFinalViewFn.address(), (void*)&EngineCameraModule::applyAdditionalOffsetForCameraViewFromBehaviourBlender);
    if(!m_onCalcFinalView) {
        spdlog::error("Failed to hook m_onCalcFinalView");
    }

    REL::Relocation<void *> getOrphoFovFn{ memory::get_orpho_fov_fn() };
    m_onGetOrphoFov = safetyhook::create_inline((void*)getOrphoFovFn.address(), (void*)&EngineCameraModule::onGetOrphoFov);
    if(!m_onGetOrphoFov) {
        spdlog::error("Failed to hook m_getOrthoFov");
    }
}

// right naded coordinate system, row major matrices
sdk::Matrix4x4* EngineCameraModule::onCalcProjection(sdk::RendererContext* a1, float scalingFactor, uint8_t flag, __int64 a4, float nearZ, float farZ, float fovYScaleFactor, float fovXScaleFactor,
                                                                sdk::Matrix4x4* viewProjOut, sdk::Matrix4x4* invViewProjOut, float* outTanHalfFovX, float* outTanHalfFovY,
                                                                sdk::Matrix4x4* viewMatrixOut) {

    static auto instance = EngineCameraModule::Get();
    static auto vr = VR::get();
//    if(!a1->cameraObjectPtr || !a1->cameraObjectPtr->pViewport->orpho) {
//        spdlog::info("[cvp:before] scalingFactor={} flag={} a4={} nearZ={} farZ={} fovYScaleFactor={} fovXScaleFactor={}", scalingFactor, flag, a4, nearZ, farZ, fovYScaleFactor,
//                     fovXScaleFactor);
//    }
    auto result = instance->m_onCalcProjection.call<sdk::Matrix4x4* >(a1, scalingFactor, flag, a4, nearZ, farZ, fovYScaleFactor, fovXScaleFactor, viewProjOut, invViewProjOut, outTanHalfFovX, outTanHalfFovY, viewMatrixOut);

    if(!vr->is_hmd_active()) {
        return result;
    }

    const auto is_left_eye = vr->get_current_render_eye() == VRRuntime::Eye::LEFT;
    if(!a1->cameraObjectPtr || a1->cameraObjectPtr->pViewport->orpho) {
        //        auto scale_factor = ModSettings::g_UISettings.fovRadius;
        //        viewProjOut->row[0][0] *= scale_factor;
        //        viewProjOut->row[1][1] *= scale_factor;
        viewProjOut->row[3][1] += ModSettings::g_UISettings.hudVerticalOffset;
        auto invViewProj = glm::inverse(*(glm::mat4*)viewProjOut);
        *invViewProjOut = *(sdk::Matrix4x4*)&invViewProj;
    }
    if(!a1->cameraObjectPtr || !a1->cameraObjectPtr->pViewport->orpho) {
        vr->m_nearz = nearZ;
        vr->m_farz = farZ;
        auto proj = vr->get_current_projection_matrix();
        *outTanHalfFovX = 1.0f / proj[0][0];
        *outTanHalfFovY = 1.0f / proj[1][1];
        proj =  proj * *(glm::mat4*) viewMatrixOut;
        *viewProjOut = *(sdk::Matrix4x4*)&proj;
        auto invProj = glm::inverse(proj);
        *invViewProjOut = *(sdk::Matrix4x4*)&invProj;
    }
    return result;
}

void EngineCameraModule::applyAdditionalOffsetForCameraViewFromBehaviourBlender(__int64 pBehaviouralCameraBlender, sdk::Matrix4x4* viewMatrixIn, float inFov,
                                                                                sdk::Matrix4x4* viewMatrixOut, float* outFov, float offset_scale)
{
    static auto instance = EngineCameraModule::Get();
    static auto vr = VR::get();
    instance->m_onCalcFinalView.call<void>(pBehaviouralCameraBlender, viewMatrixIn, inFov, viewMatrixOut, outFov, offset_scale);
#ifdef _DEBUG
    if(ModSettings::g_internalSettings.cameraShake) {
        auto offset = glm::mat4{1.0f};
        offset[3].x = 0.034f * (vr->m_engine_frame_count % 2 == 0 ? 1.0f : -1.0f);
        auto transform = *(glm::mat4*)viewMatrixOut * offset;
        *viewMatrixOut = *(sdk::Matrix4x4*)&transform;
        return;
    }
#endif
    if(!vr->is_hmd_active()) {
        return;
    }

    auto eye = vr->get_current_eye_transform();
    static auto hmd_transform = vr->get_transform(0);
    if(vr->get_current_render_eye() == VRRuntime::Eye::LEFT) {
        hmd_transform = vr->get_transform(0);
    }
    auto transform = *(glm::mat4*)viewMatrixOut * hmd_transform * eye;
    *viewMatrixOut = *(sdk::Matrix4x4*)&transform;
}

float EngineCameraModule::onGetOrphoFov(sdk::camera::Client* client){
    static auto instance = EngineCameraModule::Get();
    static auto vr = VR::get();
    auto result = instance->m_onGetOrphoFov.call<float>(client);
    if(!vr->is_hmd_active()) {
        return result;
    }
    return result / ModSettings::g_UISettings.hudScale;
}

//void EngineCameraModule::apply_hmd_transform(sdk::PerspectiveView* view, Matrix4x3f* pMat, float fov)
//{
//    static auto vr = VR::get();
//#ifdef _DEBUG
//    if(ModSettings::g_internalSettings.cameraShake) {
//        static auto frames = NorthLightEngineRendererModule::get_frame_tokens();
//        auto render_frame = vr->m_frame_count_alt;
//        glm::mat4 eye_separation = glm::mat4(1.0f);
//        eye_separation[3].x = 0.034f * (render_frame % 2 == 0 ? 1.0f : -1.0f);
//        spdlog::info("Applying camera shake for frame {} {}", render_frame, render_frame % 2 == 0? "left" : "right");
//        auto camera = glm::mat4(*pMat);
////        camera[2] = -camera[2];
//        auto new_rotation = camera * eye_separation;
//        *pMat = glm::mat4x3(new_rotation);
//        return;
//    }
//#endif
//
//    auto camera = glm::mat4(*pMat);
//    storage::gStatefullData.cameraData.last_transform = camera;
//
//    if (!vr->is_hmd_active() || ModSettings::g_game_state.shouldShowImmersiveView()) {
//        return;
//    }
//
//    if(ModSettings::g_internalSettings.aimCameraMode == ModSettings::Aim_CameraFirstPerson) {
//        if(ModSettings::g_game_state.m_current_state == AW2GameFlow::G_CombatAiming) {
//            auto player_transform = storage::gStatefullData.cameraData.rotation_snapshot;
//            player_transform[3] = storage::gStatefullData.playerData.position;
//            player_transform[3].y += ModSettings::g_internalSettings.cameraOffset.y;
//            static glm::mat4 camera_offset{1.0f};
//            camera_offset[3].x = ModSettings::g_internalSettings.cameraOffset.x;
//            camera_offset[3].z = ModSettings::g_internalSettings.cameraOffset.z;
//            player_transform *= camera_offset;
//            camera = player_transform;
//        } else {
//            storage::gStatefullData.cameraData.rotation_snapshot = camera;
//            storage::gStatefullData.cameraData.rotation_snapshot[3] = {0.0, 0.0, 0.0, 1.0};
//        }
//    } else if(ModSettings::g_internalSettings.aimCameraMode == ModSettings::Aim_CameraThirdPerson) {
//        if(ModSettings::g_game_state.m_current_state == AW2GameFlow::G_CombatAiming) {
//            camera = storage::gStatefullData.cameraData.rotation_snapshot;
//            camera[3] = storage::gStatefullData.playerData.position + storage::gStatefullData.cameraData.offset_from_player;
//        } else {
//            storage::gStatefullData.cameraData.rotation_snapshot = camera;
//            storage::gStatefullData.cameraData.rotation_snapshot[3] = {0.0, 0.0, 0.0, 1.0};
//            storage::gStatefullData.cameraData.offset_from_player = camera[3] - storage::gStatefullData.playerData.position;
//        }
//    }
//    camera[2] = -camera[2];
//    auto current_hmd_rotation = vr->get_transform(0);
//    auto standing_origin = vr->get_standing_origin();
//    current_hmd_rotation[3].x -= standing_origin.x;
//    current_hmd_rotation[3].y -= standing_origin.y;
//    current_hmd_rotation[3].z -= standing_origin.z;
//    current_hmd_rotation[2] = -current_hmd_rotation[2];
//    auto eye = vr->get_current_eye_transform();
//    auto new_rotation = camera * current_hmd_rotation * eye;
//    *pMat = glm::mat4x3(new_rotation);
//}

//typedef float* (CalcViewProjection)(sdk::PerspectiveView*);
//
//Matrix3x4d* EngineCameraModule::onSetViewAndFov(sdk::PerspectiveView* pView, Matrix4x3f* pMat, float fov, char i)
//{
//    using func_t              = decltype(onSetViewDetour);
//    static auto original_func = m_onSetViewAndFov->get_original<func_t>();
//    static auto vr = VR::get();
//    if(vr->is_hmd_active()) {
//        vr->m_nearz = pView->nearPlane;
//        vr->m_farz = pView->farPlane;
//    }
//    apply_hmd_transform(pView, pMat, fov);
//    auto result =  original_func(pView, pMat, fov, 0);
//    if(vr->is_hmd_active() && !ModSettings::g_game_state.shouldShowImmersiveView()) {
//        Matrix4x4d projection = Matrix4x4d{ vr->get_current_projection_matrix() };
//        pView->projection = projection;
//        pView->inverseProjection = glm::inverse(projection);
//        static REL::Relocation<CalcViewProjection> onCalcViewProjectionFunc{ memory::calc_view_projection() };
//        onCalcViewProjectionFunc(pView);
//    }
//    adjust_crosshair_position(pView);
//    return result;
//}

//uintptr_t EngineCameraModule::on_world_to_screen(float* out, glm::vec3* wPoint, sdk::View* view, char unk)
//{
//    static auto instance = EngineCameraModule::Get();
//    using func_t = decltype(on_world_to_screen);
//    auto original_fn = instance->m_onCalcWorldToScreen->get_original<func_t>();
//    auto response =  original_fn(out, wPoint, view, unk);
//    float offset = (1.0f - ModSettings::g_hudScale.x) / 2.0f;
//    out[0] = (out[0] - offset) / ModSettings::g_hudScale.x;
//    offset = (1.0f - ModSettings::g_hudScale.y) / 2.0f;
//    out[1] = (out[1] - offset) / ModSettings::g_hudScale.y;
//    out[0] = std::clamp(out[0], 0.01f, 0.99f);
//    out[1] = std::clamp(out[1], 0.01f, 0.99f);
//    return response;
//}
//
//float* EngineCameraModule::on_world_to_screen2(float* out, sdk::PerspectiveView* view, glm::vec3* wPoint) {
//    static auto instance = EngineCameraModule::Get();
//    auto response =  instance->m_onCalcWorldToScreen2.call<float*>(out, view, wPoint);
//    float offset = (1.0f - ModSettings::g_hudScale.x) / 2.0f;
//    out[0] = (out[0] - offset) / ModSettings::g_hudScale.x;
//    offset = (1.0f - ModSettings::g_hudScale.y) / 2.0f;
//    out[1] = (out[1] - offset) / ModSettings::g_hudScale.y;
//    out[0] = std::clamp(out[0], 0.01f, 0.99f);
//    out[1] = std::clamp(out[1], 0.01f, 0.99f);
//    return response;
//}