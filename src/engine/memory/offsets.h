#pragma once
#include "ScanHelper.h"

namespace memory {

    inline uintptr_t calc_view_projection() {
        static auto pattern = "4C 8B DC 49 89 5B 18 56 57";
        static auto addr = FuncRelocation("calc_view_projection", pattern, 0x540190);
        return addr;
    }

    inline uintptr_t calc_final_view_fn() {
        static auto pattern = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 40 48 8B F2 0F 29 74 24 30";
        static auto addr = FuncRelocation("calc_final_view_fn", pattern, 0x535dd0);
        return addr;
    }

    inline uintptr_t get_orpho_fov_fn() {
        static auto pattern = "80 B9 88 01 00 00 00 75 14 0F B6 41 52 C0 E8 04 A8 01 75 09 F3 0F 10 81 A4";
        static auto addr = FuncRelocation("get_orpho_fov_fn", pattern, 0x692730);
        return addr;
    }

    inline uintptr_t simulation_start_fn() {
        static auto pattern = "48 83 EC 28 80 3D ? ? ? ? ? 74 4F";
        static auto addr = FuncRelocation("simulation_start_fn", pattern, 0xe8d060);
        return addr;
    }

    inline uintptr_t calc_render_size_letterbox_fn() {
        static auto pattern = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B D9 0F";
        static auto addr = FuncRelocation("calc_render_size_letterbox_fn", pattern, 0x368270);
        return addr;
    }

    inline uintptr_t pcl_set_marker_pointer_fn() {
        static auto pattern = "48 8B 05 ? ? ? ? 48 8B D3 33 C9";
        static auto addr = InstructionRelocation("pcl_set_marker_pointer_fn", pattern, 3, 7, 0x60c5848);
        return addr;
    }

    typedef  void(*set_window_mode_t)(int); // 0 = windowed, 1 = fullscreen
    inline set_window_mode_t setWindowModeFn() {
        static auto pattern = "E8 ? ? ? ? 48 8B 0D ? ? ? ? 4C 8D 4D 50";
        static auto addr = InstructionRelocation("setWindowModeFn", pattern, 1, 5, 0xe8d7e0);
        return (set_window_mode_t)addr;
    }

    typedef uintptr_t (*set_aspect_ratio_t)(int); // 0 = auto, 1 = 16:9, 2 = 21:9
    inline set_aspect_ratio_t setAspectRatioFn() {
        static auto pattern = "E8 ? ? ? ? 33 C0 8B D5";
        static auto addr = InstructionRelocation("setAspectRatioFn", pattern, 1, 5, 0x900730);
        return (set_aspect_ratio_t)addr;
    }

    typedef void(*set_frame_rate_t)(int); // 0 = unlimited
    inline set_frame_rate_t setFrameRateFn() {
        static auto pattern = "E8 ? ? ? ? E8 ? ? ? ? 48 63 F8 E8 ? ? ? ? 48 8B D8 E8 ? ? ? ? 4C 8B CF 48 8D 0D ? ? ? ? 4C 8B C3 8B D0 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 15";
        static auto addr = InstructionRelocation("setFrameRateFn", pattern, 1, 5, 0xe8d450);
        return (set_frame_rate_t)addr;
    }

    typedef void(*set_vsync_t)(int);
    inline set_vsync_t setVSyncFn() {
        static auto pattern = "E8 ? ? ? ? 8B 1D ? ? ? ? E8 ? ? ? ? 4C 8B C0 48 8D 0D ? ? ? ? 41 B9 07 00 00 00 8B D3 E8 ? ? ? ? 89 05";
        static auto addr = InstructionRelocation("setVSyncFn", pattern, 1, 5, 0xe8d710);
        return (set_vsync_t)addr;
    }

    typedef uintptr_t(*set_framegen_mode_t)(int); // 0 = off, 1 = on, 2 = auto
    inline set_framegen_mode_t setFramegenModeFn() {
        static auto pattern = "E8 ? ? ? ? 8B 15 ? ? ? ? 4C 8D 05 ? ? ? ? 41 B9 05 00 00 00";
        static auto addr = InstructionRelocation("setFramegenModeFn", pattern, 1, 5, 0x368750);
        return (set_framegen_mode_t)addr;
    }

}
