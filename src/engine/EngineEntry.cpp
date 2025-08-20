#include "EngineEntry.h"
#include "EngineCameraModule.h"
#include "EngineRendererModule.h"
#include "EngineTwicks.h"
#include "models/GOWGameState.h"
#include <engine/models/ModSettings.h>
#include <engine/models/StatefullData.h>
#include <mods/VR.hpp>

std::optional<std::string> EngineEntry::on_initialize()
{
    EngineTwicks::DisableBadEffects();
    EngineRendererModule::Get()->InstallHooks();
    EngineCameraModule::Get()->InstallHooks();
//    UpscalerAfrNvidiaModule::Get();
    return Mod::on_initialize();
}

void EngineEntry::on_draw_ui()
{
    if (!ImGui::CollapsingHeader(get_name().data())) {
        return;
    }
    /*static auto instance = NVPhysXModule::Get();
    {
        auto rt_w    = (float)g_framework->get_rendertarget_width_d3d12();
        auto rt_h    = (float)g_framework->get_rendertarget_height_d3d12();
        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(rt_w,rt_h));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::Begin("Circle", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);

            auto* draw_list = ImGui::GetWindowDrawList();
            auto last_ray_pos = instance->get_last_ray_pos();
            auto hit = EngineRendererModule::world_to_screen(&last_ray_pos);
            draw_list->AddCircleFilled(ImVec2(hit.x* rt_w,hit.y * rt_h), 5.0f, ImColor(0,200,0));
            ImGui::End();
            ImGui::PopStyleVar();
        }
    }*/

    if(m_hud_scale->draw("HUD Scale"))
    {
        update(true);
    }

    if(m_hud_offset_y->draw("HUD Offset Y"))
    {
        update(true);
    }

    if(m_dlss_fix->draw("DLSS and Motion vector Fix")) {
        update(true);
    }

//    if(ImGui::Button("ChangeWindowSize"))
//    {
//        auto  hWnd      = g_framework->get_window();
//        RECT  rect      = { 0, 0, (LONG)2500, (LONG)2500 };
//        DWORD dwStyle   = GetWindowLong(hWnd, GWL_STYLE);
//        DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
//        BOOL  hasMenu   = GetMenu(hWnd) != NULL;
//        AdjustWindowRectEx(&rect, dwStyle, hasMenu, dwExStyle);
//
//        int nWidth  = rect.right - rect.left;
//        int nHeight = rect.bottom - rect.top;
//        spdlog::info("Setting window size to {} {}", nWidth, nHeight);
//        SetWindowPos(hWnd, nullptr, 0, -500, nWidth, nHeight, 0x604);
//    }

//    if(m_hud_scale_y->draw("HUD Scale Y"))
//    {
//        update(true);
//    }
//
//    if(m_display_distance->draw("Flat Screen Display Distance"))
//    {
//        ModSettings::g_internalSettings.quadDisplayDistance = m_display_distance->value();
//    }
//    if(m_force_flat_screen->draw("Force Flat Screen"))
//    {
//        ModSettings::g_game_state.enforceFlatScreen = m_force_flat_screen->value();
//        update(true);
//    }
//    if(m_motion_control_magnifier_horizontal->draw("Motion Control Magnifier Horizontal"))
//    {
//        ModSettings::g_internalSettings.motionControlSensitivityH = m_motion_control_magnifier_horizontal->value();
//    }
//
//    if(m_motion_control_magnifier_vertical->draw("Motion Control Magnifier Vertical"))
//    {
//        ModSettings::g_internalSettings.motionControlSensitivityV = m_motion_control_magnifier_vertical->value();
//    }
//
//    ImGui::InputFloat3("Camera Offset", (float*) &ModSettings::g_internalSettings.cameraOffset);
//
//    if(m_aim_mode->draw("Aim Mode"))
//    {
//        ModSettings::g_internalSettings.aimCameraMode = m_aim_mode->value();
//    }
//
//    if(m_left_handed_controls->draw("Left Handed Controls"))
//    {
//        ModSettings::g_internalSettings.leftHandedControls = m_left_handed_controls->value();
//    }
//
//    /*if(m_world_scale->draw("World Scale"))
//    {
//        ModSettings::g_internalSettings.worldScale = m_world_scale->value();
//    }*/
//
//#if 0
//
//    struct DebugActorInfo {
//        int index;
//        glm::vec3 location;
//        float mass;
//        glm::vec3 minBB;
//        glm::vec3 maxBB;
//    };
//
//    static std::vector<DebugActorInfo> g_debugActors;
//    static float filter_mass = 1.0f;
//    ImGui::InputFloat("Filter Mass", &filter_mass);
//
//    static auto offset = glm::mat4{1.0f};
//    offset[3].z = 1.0f;
//
//
//    {
//        static auto instance = NorthLightEngineCameraModule::Get();
//        auto wts = [&] (float* out, glm::vec3* wPoint) {
//            static REL::Relocation<sdk::View*> g_projection{ memory::g_projection_view() };
//            instance->m_onCalcWorldToScreen2.call<float*>(out, g_projection, wPoint);
//        };
//        auto rt_w    = (float)g_framework->get_rendertarget_width_d3d12();
//        auto rt_h    = (float)g_framework->get_rendertarget_height_d3d12();
//        {
//            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
//            ImGui::SetNextWindowSize(ImVec2(rt_w, rt_h));
//            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
//            ImGui::Begin("Circle", nullptr,
//                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
//
//            auto* draw_list = ImGui::GetWindowDrawList();
//            auto& transforms = ModSettings::g_debugAndCalibration.debugVectors;
//            int index = 0;
//            for (auto& transform : transforms) {
//                auto from = glm::vec3(transform[3]);
//                auto to   = glm::vec3((transform * offset)[3]);
//                ImVec2 point_from{};
//                ImVec2 point_to{};
//                wts(&point_from.x, &from);
//                wts(&point_to.x, &to);
//
//                // Draw filled circle in red
//                point_from.x *= rt_w;
//                point_from.y *= rt_h;
//                point_to.x *= rt_w;
//                point_to.y *= rt_h;
//                draw_list->AddLine(point_from, point_to, IM_COL32(0, 255, 0, 255), 1.0f);
//                auto id = ModSettings::g_debugAndCalibration.debugVectorsID[index];
//                char label[32];
//                sprintf_s(label, "[%d]", id);
//                draw_list->AddText(ImVec2(point_from.x, point_from.y + 5), IM_COL32(255, 255, 255, 255), label);
//
//                index++;
//            }
//        }
//
//        ImGui::End();
//        ImGui::PopStyleVar();
//        ModSettings::g_debugAndCalibration.debugVectors.clear();
//        ModSettings::g_debugAndCalibration.debugVectorsID.clear();
//    }
//
//    if(ImGui::Button("Debug Scene Actors")) {
//        g_debugActors.clear(); // reset the list
////        static auto physx          = sdk::physics::getMainPhysXScene();
////        auto p_scene = physx->scene->scene;
////        auto actor_count = p_scene->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC);
////        auto actors = new physx::PxActor*[actor_count];
////        p_scene->getActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC, actors, actor_count, 0);
//        static auto camera_module = NorthLightEngineCameraModule::Get();
//        auto cameraPos = glm::vec3(storage::gStatefullData.cameraData.last_transform[3]);
//        auto same_vtable = std::unordered_set<uintptr_t>{};
//        int i = 0;
//        for(auto actor:          ModSettings::g_debugAndCalibration.debugActors) {
////            auto actor = (physx::PxRigidDynamic*)actors[i];
//            auto bounds = actor->getWorldBounds();
//            auto location = actor->getGlobalPose().p;
//            auto mass = actor->getMass();
//            // Filter out actors closer than 2 meters
//            /*float distance = glm::length(glm::vec3(location.x, location.y, location.z) - cameraPos);
//            if (distance > 3.0f) {
//                continue;
//            }
//            if (filter_mass > 0.0f && mass != filter_mass) {
//                continue;
//            }*/
//            DebugActorInfo info{};
//            info.location = { location.x, location.y, location.z };
//            info.mass     = mass;
//            info.index    = i++;
//            info.minBB    = { bounds.minimum.x, bounds.minimum.y, bounds.minimum.z };
//            info.maxBB    = { bounds.maximum.x, bounds.maximum.y, bounds.maximum.z };
//            g_debugActors.push_back(info);
//        }
//    }
//
//    if (0) {
//        auto rt_w    = (float)g_framework->get_rendertarget_width_d3d12();
//        auto rt_h    = (float)g_framework->get_rendertarget_height_d3d12();
//        {
//            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
//            ImGui::SetNextWindowSize(ImVec2(rt_w,rt_h));
//            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
//            ImGui::Begin("Circle", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
//
//            auto* draw_list = ImGui::GetWindowDrawList();
//            static auto instance = NorthLightEngineCameraModule::Get();
//
//            auto wts = [&] (float* out, glm::vec3* wPoint) {
//                static REL::Relocation<sdk::View*> g_projection{ memory::g_projection_view() };
//                instance->m_onCalcWorldToScreen2.call<float*>(out, g_projection, wPoint);
//            };
//            for(auto& actor : g_debugActors) {
//                glm::vec3 bboxSize = actor.maxBB - actor.minBB;
//
//                // Exclude actors whose bounding box exceeds 1 meter in any dimension
////                if (bboxSize.x > 1.f || bboxSize.y > 1.f || bboxSize.z > 1.f) {
////                    continue;
////                }
//                float clamped = 5 * std::logf(actor.mass);
//                clamped = std::max(clamped, 1.0f);
//                ImVec2 screenPos{};
//                wts(&screenPos.x, &actor.location);
//
//                // Draw filled circle in red
//                screenPos.x *= rt_w;
//                screenPos.y *= rt_h;
//                draw_list->AddCircleFilled(screenPos, clamped, IM_COL32(255, 0, 0, 255));
//
//                char label[32];
//                sprintf_s(label, "[%d]%.1f", actor.index, actor.mass);
//                draw_list->AddText(ImVec2(screenPos.x + clamped + 4, screenPos.y), IM_COL32(255, 255, 255, 255), label);
//
//                // Draw bounding box lines
//                glm::vec3 corners[8] = {
//                    { actor.minBB.x, actor.minBB.y, actor.minBB.z },
//                    { actor.minBB.x, actor.minBB.y, actor.maxBB.z },
//                    { actor.minBB.x, actor.maxBB.y, actor.minBB.z },
//                    { actor.minBB.x, actor.maxBB.y, actor.maxBB.z },
//                    { actor.maxBB.x, actor.minBB.y, actor.minBB.z },
//                    { actor.maxBB.x, actor.minBB.y, actor.maxBB.z },
//                    { actor.maxBB.x, actor.maxBB.y, actor.minBB.z },
//                    { actor.maxBB.x, actor.maxBB.y, actor.maxBB.z }
//                };
//
//                // Project corners
//                ImVec2 screenCorners[8];
//                for (int c = 0; c < 8; c++) {
//                    wts(&screenCorners[c].x, &corners[c]);
//                    screenCorners[c].x *= rt_w;
//                    screenCorners[c].y *= rt_h;
//                }
//
//                // Define edges of the AABB for drawing lines
//                static const int edges[12][2] = {
//                    { 0, 1 },
//                    { 0, 2 },
//                    { 1, 3 },
//                    { 2, 3 },
//                    { 4, 5 },
//                    { 4, 6 },
//                    { 5, 7 },
//                    { 6, 7 },
//                    { 0, 4 },
//                    { 1, 5 },
//                    { 2, 6 },
//                    { 3, 7 }
//                };
//
//                // Draw bounding box edges
//                for (const auto& e : edges) {
//                    draw_list->AddLine(screenCorners[e[0]], screenCorners[e[1]], IM_COL32(0, 255, 0, 255), 1.0f);
//                }
//            }
//            ImGui::End();
//            ImGui::PopStyleVar();
//        }
//    }
//
//    {
//        static auto camera_module = NorthLightEngineCameraModule::Get();
//        auto original = storage::gStatefullData.cameraData.last_transform;
//        original[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//        glm::mat4 invView = glm::inverse(original);
//        glm::vec3 forward = glm::normalize(glm::vec3(invView[2]));
//        glm::vec3 up = glm::normalize(glm::vec3(invView[1]));
//        float outYaw = glm::degrees(std::atan2(forward.x, forward.z));
//        float outPitch = glm::degrees(std::atan2(up.z, up.y));
//
//        auto euler = glm::eulerAngles(glm::quat_cast(original));
//
//
//        ImGui::Text("Player Rotation: %f %f", outYaw, outPitch);
//        ImGui::Text("Player Rotation Euler: %f %f %f", glm::degrees(euler.x), glm::degrees(euler.y), glm::degrees(euler.z));
//
//        {
//
//            auto mean = ModSettings::g_debugAndCalibration.cumulative / (float)ModSettings::g_debugAndCalibration.counter;
//            ImGui::Text("Mean Rotation Difference: %f %f %f", mean.x, mean.y, mean.z);
//            ImGui::Text("Coef yaw=%f pitch=%f", ModSettings::g_debugAndCalibration.staticYaw/mean.x, ModSettings::g_debugAndCalibration.staticPitch/mean.y);
//            if(AW2GameFlow::G_CombatAiming != ModSettings::g_game_state.m_current_state) {
//                ModSettings::g_debugAndCalibration.counter = 0;
//                ModSettings::g_debugAndCalibration.cumulative = glm::vec3{};
//            }
//        }
//        ImGui::Checkbox("Calibrate Joystick", &ModSettings::g_debugAndCalibration.calibrateJoystick);
//        ImGui::InputFloat2("Static Yaw Pitch", &ModSettings::g_debugAndCalibration.staticYaw);
//
//    }
//
//    {
//        static auto vr = VR::get();
//        auto joy_rotation         = vr->get_rotation(2);
//        auto current_joy_rotation = glm::quat{ joy_rotation };
//        glm::mat4 test = joy_rotation;
//        test[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//        auto invView =  test;
//        auto forward = glm::normalize(glm::vec3(invView[2]));
//        auto up = glm::normalize(glm::vec3(invView[1]));
//        auto outYaw = glm::degrees(std::atan2(forward.x, forward.z));
//        auto outPitch = glm::degrees(std::atan2(up.z, up.y));
//        ImGui::Text("Joy Rotation: %f %f", outYaw, outPitch);
//
//    }
//
//
//    {
//        static auto camera_module = NorthLightEngineCameraModule::Get();
//        auto player_origin = storage::gStatefullData.playerData.position;
//        auto camera_origin = storage::gStatefullData.cameraData.last_transform[3];
//        ImGui::Text("Camera Origin: %f %f %f", camera_origin.x, camera_origin.y, camera_origin.z);
//        ImGui::Text("Player Origin: %f %f %f", player_origin.x, player_origin.y, player_origin.z);
//        ImGui::Text("Camera distance: %f", glm::distance(camera_origin, player_origin));
//    }
//
#ifdef _DEBUG
    if(ImGui::Checkbox("Debug Motion Vector", &ModSettings::g_internalSettings.debugShaders))
    {
        spdlog::info("Debug Motion Vector: {}", ModSettings::g_internalSettings.debugShaders);
    }
    if(ImGui::Checkbox("Camera Shake", &ModSettings::g_internalSettings.cameraShake))
    {
        spdlog::info("Camera Shake: {}", ModSettings::g_internalSettings.cameraShake);
    }

    if (ImGui::CollapsingHeader("Debug Matrices")) {
        auto& debug = ModSettings::g_debugAndCalibration;

        // Helper lambda for displaying 4x4 matrices
        auto    displayMatrix = [](const char* name, const glm::mat4& matrix) {
            if (ImGui::CollapsingHeader(name)) {
                ImGui::Columns(4, name, true);
                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        ImGui::Text("%.3f", matrix[col][row]);
                        if (col < 3) ImGui::NextColumn();
                    }
                    if (row < 3) ImGui::NextColumn();
                }
                ImGui::Columns(1);
            }
        };

        static auto vr = VR::get();

//        displayMatrix("Camera To Clip Matrix", debug.cameraToClip);
//        displayMatrix("Camera View Matrix", debug.cameraView);
//        displayMatrix("View Matrix", ModSettings::getView(vr->m_presenter_frame_count));
//        displayMatrix("Projection Matrix", ModSettings::getProjection(vr->m_presenter_frame_count));
    }

#endif

    // Add matrix visualization

}

void EngineEntry::on_config_load(const utility::Config& cfg)
{
    for (IModValue& option : m_options) {
        option.config_load(cfg);
    }
    update(true);
}

void EngineEntry::on_config_save(utility::Config& cfg)
{
    for (IModValue& option : m_options) {
        option.config_save(cfg);
    }
}

void EngineEntry::update(bool force) {
    ModSettings::g_UISettings.hudScale = m_hud_scale->value();
    ModSettings::g_UISettings.hudVerticalOffset = m_hud_offset_y->value();
    ModSettings::g_internalSettings.nvidiaMotionVectorFix = m_dlss_fix->value();
}
