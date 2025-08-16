//
// Created by sergp on 6/26/2024.
//

#include "EngineRendererModule.h"
#include "EngineEntry.h"
#include "REL/Relocation.h"
#include "engine/memory/offsets.h"
#include <engine/models/ModSettings.h>
#include <mods/VR.hpp>
#include <safetyhook/easy.hpp>

void EngineRendererModule::InstallHooks()
{
//    static HMODULE p_hm_sl_interposter = nullptr;
//    if (p_hm_sl_interposter) {
//        return;
//    }
    spdlog::info("Installing sl.interposer hooks");
//    while ((p_hm_sl_interposter = GetModuleHandle("sl.pcl.dll")) == nullptr) {
//        std::this_thread::yield();
//    }

    //    auto getNewFrameTokenFn = GetProcAddress(g_interposer, "slGetNewFrameToken");
    //    m_get_new_frame_token_hook = std::make_unique<FunctionHook>((void **)getNewFrameTokenFn, (void *) &DlssDualView::on_slGetNewFrameToken);
    //    if(!m_get_new_frame_token_hook->create()) {
    //        spdlog::error("Failed to hook slGetNewFrameToken");
    //    }

    auto simStartFunct = memory::simulation_start_fn();
    m_simulationStartMarker = safetyhook::create_inline((void*)simStartFunct, (void*)&EngineRendererModule::onSimulationStartMarker);

    auto letterBoxFn = memory::calc_render_size_letterbox_fn();
    m_letterBoxHook = safetyhook::create_inline((void*)letterBoxFn, (void*)&EngineRendererModule::onCalcRenderSize);

//    auto slPCLSetMarkerFn = GetProcAddress(p_hm_sl_interposter, "slPCLSetMarker");
//    m_slPCLSetMarker  = std::make_unique<FunctionHook>((void**)slPCLSetMarkerFn, (void*)&EngineRendererModule::onSlPCLSetMarker);
//    if (!m_slPCLSetMarker->create()) {
//        spdlog::error("Failed to hook slPCLSetMarker");
//    }


}

void EngineRendererModule::setWindowSize()
{
    if (!g_framework->is_ready()) {
        return;
    }
    static auto vr = VR::get();
    if (!vr->is_hmd_active()) {
        return;
    }

    static std::atomic<bool> inside_change{ false };
    static int               last_synced_frame{ -1500 };
    if (inside_change.exchange(true)) {
        return;
    }

    if (!vr->is_hmd_active() || (vr->m_engine_frame_count) - last_synced_frame < 5 * 72) {
        inside_change.store(false);
        return;
    }
    last_synced_frame = vr->m_engine_frame_count;
    inside_change.store(false);

    auto        window_width    = (int)vr->get_hmd_width();
    auto        window_height   = (int)vr->get_hmd_height();
    const auto& backbuffer_size = vr->get_backbuffer_size();

    if (abs(window_width - (int)backbuffer_size[0]) < 3 && abs(window_height - (int)backbuffer_size[1]) < 3) {
        return;
    }

    auto  hWnd      = g_framework->get_window();
    RECT  rect      = { 0, 0, (LONG)window_width, (LONG)window_height };
    DWORD dwStyle   = GetWindowLong(hWnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    BOOL  hasMenu   = GetMenu(hWnd) != NULL;
    AdjustWindowRectEx(&rect, dwStyle, hasMenu, dwExStyle);

    int nWidth  = rect.right - rect.left;
    int nHeight = rect.bottom - rect.top;
    spdlog::info("Setting window size to {} {}", nWidth, nHeight);
    SetWindowPos(hWnd, nullptr, 0, 0, nWidth, nHeight, 0x604);
}


void EngineRendererModule::onSimulationStartMarker() {
    static auto addr = memory::pcl_set_marker_pointer_fn();
    static auto instance = Get();
    static bool dynamic_reflex_set_marker = false;
    if(!dynamic_reflex_set_marker && *(uintptr_t*)addr != 0) {
        dynamic_reflex_set_marker = true;
        instance->m_slPCLSetMarker2 = std::make_unique<PointerHook>((void**)addr, (void*)&EngineRendererModule::onSlPCLSetMarker);
    }
    instance->m_simulationStartMarker.call();
}

uintptr_t EngineRendererModule::onSlPCLSetMarker(uint32_t marker, const sl::FrameToken& frame)
{
    static auto instance = Get();
    static bool engine_notified = false;
    static     auto        vr            = VR::get();
    static bool            sync_marker_started = false;
    static int frames_since_reset = 0;
//    spdlog::info("onSlPCLSetMarker m={} f={}", marker, (int)frame);
    if ((marker == 0 || marker == 1) && !engine_notified) {
        vr->m_engine_frame_count = (int)frame;
        engine_notified = true;
        vr->on_engine_tick(nullptr);
    }
    // Reset notification if marker is 1
    if (marker == 1) {
//        vr->on_engine_tick(nullptr);
        engine_notified = false;
    }

    if(marker == 2) {
        vr->m_render_frame_count = (int)frame;
        vr->on_begin_rendering(nullptr);
    }
    if(marker == 4) {
        vr->m_presenter_frame_count = (int)frame;
    }

    if(marker == 2) {
        g_framework->run_imgui_frame(false);
    }

    if (marker == 3) {
        instance->setWindowSize();
    }
    static auto original_fn = instance->m_slPCLSetMarker2->get_original<decltype(EngineRendererModule::onSlPCLSetMarker)*>();
    return original_fn(marker, frame);
}


//uintptr_t EngineRendererModule::onSlPCLSetMarker(uint32_t marker, const sl::FrameToken& frame)
//{
//    static auto instance = Get();
//    static bool engine_notified = false;
//    static     auto        vr            = VR::get();
//    static bool            sync_marker_started = false;
//    static int frames_since_reset = 0;
//    spdlog::info("onSlPCLSetMarker m={} f={} en_f={} r_f={}", marker, (int)frame, vr->m_engine_frame_count, vr->m_presenter_frame_count);
//    if ((marker == 0 || marker == 1) && !engine_notified) {
//        engine_notified = true;
//        vr->on_engine_tick(nullptr);
//        if(vr->get_runtime()->loaded) {
//            frames_since_reset++;
//            if(vr->m_engine_frame_count % 2 == 0) {
//                spdlog::info("Sync frame started m={} f={} en_f={} r_f={}", marker, (int)frame, vr->m_engine_frame_count, vr->m_presenter_frame_count);
//                sync_marker_started = true;
//            } else {
//                sync_marker_started = false;
//            }
//        }
//    }
//    // Reset notification if marker is 1
//    if (marker == 1) {
//        engine_notified = false;
//    }
//
//    if(marker == 2) {
//        g_framework->run_imgui_frame(false);
//    }
//
//    if (marker == 3) {
//        instance->setWindowSize();
//    }
//
//
//    if(vr->get_runtime()->loaded && marker == 1 && sync_marker_started) {
//        /*
//         * as we sync on game loop L eye + frame + game loop R eye + frame Enc
//         * we don't expect any async frames comes into this loop
//         * if we detect async frame we reset sync and let engine to handle it
//         */
//        sync_marker_started = false;
//    } else if(frames_since_reset > 100 && marker > 1 && marker < 5 && sync_marker_started && vr->get_runtime()->loaded) {
//        spdlog::info("Detected frame inconsistency, resetting frame sync m={} f={} en_f={} r_f={}", marker, (int)frame, vr->m_engine_frame_count, vr->m_presenter_frame_count);
//        vr->m_skip_frames = 1;
//        frames_since_reset = 0;
//        sync_marker_started = false;
//    }
//    static auto original_fn = instance->m_slPCLSetMarker2->get_original<decltype(EngineRendererModule::onSlPCLSetMarker)*>();
//    return original_fn(marker, frame);
//}

char EngineRendererModule::onCalcRenderSize(int* resolutions, float aspectRatio, uint8_t* recreateGraph, float* scale)
{
    static auto instance = Get();
    static int tick = 0;
    static auto vr = VR::get();
    if(vr->is_hmd_active()) {
//        spdlog::info("resolutions[0] = {}, resolutions[1] = {}, resolutions[2] = {}, resolutions[3] = {}, resolutions[4] = {}, resolutions[5] = {}, aspctRatip = {}",
//                     resolutions[0], resolutions[1], resolutions[2], resolutions[3], resolutions[4], resolutions[5], aspectRatio);
//        resolutions[0] = 2500;
        resolutions[1] = resolutions[5];
        resolutions[3] = resolutions[5];

//        resolutions[2] = 2500;
//        resolutions[3] = 2500;
//        resolutions[4] = 2500;
//        resolutions[5] = 2500;
        aspectRatio = (float)resolutions[4]/(float)resolutions[5];
    }
    if(g_framework->is_ready()) {
        tick++;
    }

    return instance->m_letterBoxHook.call<char>(resolutions, aspectRatio, recreateGraph, scale);
}
