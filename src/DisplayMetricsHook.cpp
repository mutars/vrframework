#include "DisplayMetricsHook.hpp"

#include <atomic>
#include <mutex>

#include <intrin.h>

#include <spdlog/spdlog.h>

#include "Framework.hpp"
#include "mods/VR.hpp"
#include "safetyhook/easy.hpp"
#include "utility/Module.hpp"

namespace {
    DisplayMetricsHook* g_display_metrics_hook{ nullptr };
    std::recursive_mutex g_display_metrics_mutex{};

    // Caller gating cache.
    std::once_flag g_game_module_once{};
    uintptr_t g_game_base{};
    size_t g_game_size{};

    constexpr DWORD ENUM_CURRENT_SETTINGS_DWORD = static_cast<DWORD>(-1);
    constexpr DWORD ENUM_REGISTRY_SETTINGS_DWORD = static_cast<DWORD>(-2);

    void init_game_module_range_once() {
        std::call_once(g_game_module_once, []() {
            auto base = ::GetModuleHandleW(nullptr);
            if (base == nullptr) {
                return;
            }

            const auto size_opt = utility::get_module_size(base);
            if (!size_opt) {
                return;
            }

            g_game_base = reinterpret_cast<uintptr_t>(base);
            g_game_size = static_cast<size_t>(*size_opt);
        });
    }

    bool address_in_game_module(void* address) {
        init_game_module_range_once();

        if (g_game_base == 0 || g_game_size == 0) {
            return false;
        }

        const auto addr = reinterpret_cast<uintptr_t>(address);
        return addr >= g_game_base && addr < (g_game_base + g_game_size);
    }

    bool set_dev_mode_dimensions(DEVMODEW* devMode, uint32_t w, uint32_t h) {
        if (devMode == nullptr) {
            return false;
        }

        devMode->dmPelsWidth = w;
        devMode->dmPelsHeight = h;
        devMode->dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT;
        return true;
    }

    bool set_dev_mode_dimensions(DEVMODEA* devMode, uint32_t w, uint32_t h) {
        if (devMode == nullptr) {
            return false;
        }

        devMode->dmPelsWidth = w;
        devMode->dmPelsHeight = h;
        devMode->dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT;
        return true;
    }

    bool should_override_system_metric(int nIndex) {
        switch (nIndex) {
        case SM_CXSCREEN:
        case SM_CYSCREEN:
        case SM_CXVIRTUALSCREEN:
        case SM_CYVIRTUALSCREEN:
        case SM_CXFULLSCREEN:
        case SM_CYFULLSCREEN:
        case SM_CXMAXIMIZED:
        case SM_CYMAXIMIZED:
        case SM_CXMAXTRACK:
        case SM_CYMAXTRACK:
            return true;
        default:
            return false;
        }
    }

    std::optional<int> override_system_metric(int nIndex, uint32_t w, uint32_t h) {
        switch (nIndex) {
        case SM_CXSCREEN:
        case SM_CXVIRTUALSCREEN:
        case SM_CXFULLSCREEN:
        case SM_CXMAXIMIZED:
        case SM_CXMAXTRACK:
            return static_cast<int>(w);
        case SM_CYSCREEN:
        case SM_CYVIRTUALSCREEN:
        case SM_CYFULLSCREEN:
        case SM_CYMAXIMIZED:
        case SM_CYMAXTRACK:
            return static_cast<int>(h);
        default:
            return std::nullopt;
        }
    }

    bool should_override_device_cap(int index) {
        switch (index) {
        case HORZRES:
        case VERTRES:
        case DESKTOPHORZRES:
        case DESKTOPVERTRES:
            return true;
        default:
            return false;
        }
    }

    std::optional<int> override_device_cap(int index, uint32_t w, uint32_t h) {
        switch (index) {
        case HORZRES:
        case DESKTOPHORZRES:
            return static_cast<int>(w);
        case VERTRES:
        case DESKTOPVERTRES:
            return static_cast<int>(h);
        default:
            return std::nullopt;
        }
    }
}

DisplayMetricsHook::DisplayMetricsHook() {
    std::lock_guard _{ g_display_metrics_mutex };

    g_display_metrics_hook = this;

    auto user32 = ::GetModuleHandleA("user32.dll");
    auto gdi32 = ::GetModuleHandleA("gdi32.dll");

    if (user32 == nullptr) {
        user32 = ::LoadLibraryA("user32.dll");
    }

    if (gdi32 == nullptr) {
        gdi32 = ::LoadLibraryA("gdi32.dll");
    }

    if (user32 == nullptr || gdi32 == nullptr) {
        spdlog::error("[DisplayMetricsHook] Failed to load user32/gdi32");
        return;
    }

    auto get_system_metrics_fn = ::GetProcAddress(user32, "GetSystemMetrics");
    if (get_system_metrics_fn != nullptr) {
        m_get_system_metrics = safetyhook::create_inline((void*)get_system_metrics_fn, onGetSystemMetrics);
    }

    auto get_system_metrics_for_dpi_fn = ::GetProcAddress(user32, "GetSystemMetricsForDpi");
    if (get_system_metrics_for_dpi_fn != nullptr) {
        m_get_system_metrics_for_dpi = safetyhook::create_inline((void*)get_system_metrics_for_dpi_fn, onGetSystemMetricsForDpi);
    }

    auto get_dpi_for_window_fn = ::GetProcAddress(user32, "GetDpiForWindow");
    if (get_dpi_for_window_fn != nullptr) {
        m_get_dpi_for_window = safetyhook::create_inline((void*)get_dpi_for_window_fn, onGetDpiForWindow);
    }

    auto get_dpi_for_system_fn = ::GetProcAddress(user32, "GetDpiForSystem");
    if (get_dpi_for_system_fn != nullptr) {
        m_get_dpi_for_system = safetyhook::create_inline((void*)get_dpi_for_system_fn, onGetDpiForSystem);
    }

    auto enum_display_settings_w_fn = ::GetProcAddress(user32, "EnumDisplaySettingsW");
    if (enum_display_settings_w_fn != nullptr) {
        m_enum_display_settings_w = safetyhook::create_inline((void*)enum_display_settings_w_fn, onEnumDisplaySettingsW);
    }

    auto enum_display_settings_ex_w_fn = ::GetProcAddress(user32, "EnumDisplaySettingsExW");
    if (enum_display_settings_ex_w_fn != nullptr) {
        m_enum_display_settings_ex_w = safetyhook::create_inline((void*)enum_display_settings_ex_w_fn, onEnumDisplaySettingsExW);
    }

    auto enum_display_settings_a_fn = ::GetProcAddress(user32, "EnumDisplaySettingsA");
    if (enum_display_settings_a_fn != nullptr) {
        m_enum_display_settings_a = safetyhook::create_inline((void*)enum_display_settings_a_fn, onEnumDisplaySettingsA);
    }

    auto enum_display_settings_ex_a_fn = ::GetProcAddress(user32, "EnumDisplaySettingsExA");
    if (enum_display_settings_ex_a_fn != nullptr) {
        m_enum_display_settings_ex_a = safetyhook::create_inline((void*)enum_display_settings_ex_a_fn, onEnumDisplaySettingsExA);
    }

    auto get_device_caps_fn = ::GetProcAddress(gdi32, "GetDeviceCaps");
    if (get_device_caps_fn != nullptr) {
        m_get_device_caps = safetyhook::create_inline((void*)get_device_caps_fn, onGetDeviceCaps);
    }

    auto monitor_from_window_fn = ::GetProcAddress(user32, "MonitorFromWindow");
    if (monitor_from_window_fn != nullptr) {
        m_monitor_from_window = safetyhook::create_inline((void*)monitor_from_window_fn, onMonitorFromWindow);
    }

    auto get_monitor_info_w_fn = ::GetProcAddress(user32, "GetMonitorInfoW");
    if (get_monitor_info_w_fn != nullptr) {
        m_get_monitor_info_w = safetyhook::create_inline((void*)get_monitor_info_w_fn, onGetMonitorInfoW);
    }

    auto get_monitor_info_a_fn = ::GetProcAddress(user32, "GetMonitorInfoA");
    if (get_monitor_info_a_fn != nullptr) {
        m_get_monitor_info_a = safetyhook::create_inline((void*)get_monitor_info_a_fn, onGetMonitorInfoA);
    }

    auto enum_display_monitors_fn = ::GetProcAddress(user32, "EnumDisplayMonitors");
    if (enum_display_monitors_fn != nullptr) {
        m_enum_display_monitors = safetyhook::create_inline((void*)enum_display_monitors_fn, onEnumDisplayMonitors);
    }

    auto get_window_placement_fn = ::GetProcAddress(user32, "GetWindowPlacement");
    if (get_window_placement_fn != nullptr) {
        m_get_window_placement = safetyhook::create_inline((void*)get_window_placement_fn, onGetWindowPlacement);
    }

    m_valid = true;
    spdlog::info("[DisplayMetricsHook] Installed Win32/GDI display metric hooks");
}

DisplayMetricsHook::~DisplayMetricsHook() {
    std::lock_guard _{ g_display_metrics_mutex };
    g_display_metrics_hook = nullptr;
}

bool DisplayMetricsHook::is_game_caller() {
    // Avoid spoofing overlays/UI frameworks inside the process.
    // Gate to calls originating from the main game module.
    return address_in_game_module(_ReturnAddress());
}

bool DisplayMetricsHook::is_target_game_window(HWND hwnd) {
    if (hwnd == nullptr) {
        return false;
    }

    // If the framework already knows the real game window, only spoof that one.
    if (g_framework != nullptr) {
        const auto game_wnd = g_framework->get_window();
        if (game_wnd != nullptr && hwnd != game_wnd) {
            return false;
        }
    }

    return true;
}

std::optional<std::pair<uint32_t, uint32_t>> DisplayMetricsHook::get_spoof_resolution() {
    if (!is_game_caller()) {
        return std::nullopt;
    }

    // Only spoof when HMD is active.
    const auto vr = VR::get();
    if (!vr->is_hmd_active()) {
        return std::nullopt;
    }

    const auto w = vr->get_hmd_width();
    const auto h = vr->get_hmd_height();

    if (w == 0 || h == 0) {
        return std::nullopt;
    }

    return std::make_pair(w, h);
}

std::optional<std::pair<uint32_t, uint32_t>> DisplayMetricsHook::get_spoof_resolution_for_hwnd(HWND hwnd) {
    if (!is_target_game_window(hwnd)) {
        return std::nullopt;
    }

    return get_spoof_resolution();
}

int WINAPI DisplayMetricsHook::onGetSystemMetrics(int nIndex) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_system_metrics) {
        return 0;
    }

    const auto original = g_display_metrics_hook->m_get_system_metrics.call<int>(nIndex);

    if (!should_override_system_metric(nIndex)) {
        return original;
    }

    const auto res = get_spoof_resolution();
    if (!res) {
        return original;
    }

    if (const auto overridden = override_system_metric(nIndex, res->first, res->second); overridden) {
        return *overridden;
    }

    return original;
}

int WINAPI DisplayMetricsHook::onGetSystemMetricsForDpi(int nIndex, UINT dpi) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_system_metrics_for_dpi) {
        return 0;
    }

    const auto original = g_display_metrics_hook->m_get_system_metrics_for_dpi.call<int>(nIndex, dpi);

    if (!should_override_system_metric(nIndex)) {
        return original;
    }

    const auto res = get_spoof_resolution();
    if (!res) {
        return original;
    }

    if (const auto overridden = override_system_metric(nIndex, res->first, res->second); overridden) {
        return *overridden;
    }

    return original;
}

UINT WINAPI DisplayMetricsHook::onGetDpiForWindow(HWND hwnd) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_dpi_for_window) {
        return USER_DEFAULT_SCREEN_DPI;
    }

    // DPI spoofing is intentionally not done right now; we just need consistency.
    // The callsite gating still applies so we can add overrides later if needed.
    return g_display_metrics_hook->m_get_dpi_for_window.call<UINT>(hwnd);
}

UINT WINAPI DisplayMetricsHook::onGetDpiForSystem() {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_dpi_for_system) {
        return USER_DEFAULT_SCREEN_DPI;
    }

    return g_display_metrics_hook->m_get_dpi_for_system.call<UINT>();
}

BOOL WINAPI DisplayMetricsHook::onEnumDisplaySettingsW(LPCWSTR deviceName, DWORD iModeNum, DEVMODEW* devMode) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_enum_display_settings_w) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_enum_display_settings_w.call<BOOL>(deviceName, iModeNum, devMode);

    // Post-process "current" or "registry" modes so other DEVMODE fields remain consistent.
    if (ok && (iModeNum == ENUM_CURRENT_SETTINGS_DWORD || iModeNum == ENUM_REGISTRY_SETTINGS_DWORD)) {
        if (const auto res = get_spoof_resolution(); res) {
            set_dev_mode_dimensions(devMode, res->first, res->second);
        }
    }

    return ok;
}

BOOL WINAPI DisplayMetricsHook::onEnumDisplaySettingsExW(LPCWSTR deviceName, DWORD iModeNum, DEVMODEW* devMode, DWORD flags) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_enum_display_settings_ex_w) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_enum_display_settings_ex_w.call<BOOL>(deviceName, iModeNum, devMode, flags);

    if (ok && (iModeNum == ENUM_CURRENT_SETTINGS_DWORD || iModeNum == ENUM_REGISTRY_SETTINGS_DWORD)) {
        if (const auto res = get_spoof_resolution(); res) {
            set_dev_mode_dimensions(devMode, res->first, res->second);
        }
    }

    return ok;
}

BOOL WINAPI DisplayMetricsHook::onEnumDisplaySettingsA(LPCSTR deviceName, DWORD iModeNum, DEVMODEA* devMode) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_enum_display_settings_a) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_enum_display_settings_a.call<BOOL>(deviceName, iModeNum, devMode);

    if (ok && (iModeNum == ENUM_CURRENT_SETTINGS_DWORD || iModeNum == ENUM_REGISTRY_SETTINGS_DWORD)) {
        if (const auto res = get_spoof_resolution(); res) {
            set_dev_mode_dimensions(devMode, res->first, res->second);
        }
    }

    return ok;
}

BOOL WINAPI DisplayMetricsHook::onEnumDisplaySettingsExA(LPCSTR deviceName, DWORD iModeNum, DEVMODEA* devMode, DWORD flags) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_enum_display_settings_ex_a) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_enum_display_settings_ex_a.call<BOOL>(deviceName, iModeNum, devMode, flags);

    if (ok && (iModeNum == ENUM_CURRENT_SETTINGS_DWORD || iModeNum == ENUM_REGISTRY_SETTINGS_DWORD)) {
        if (const auto res = get_spoof_resolution(); res) {
            set_dev_mode_dimensions(devMode, res->first, res->second);
        }
    }

    return ok;
}

int WINAPI DisplayMetricsHook::onGetDeviceCaps(HDC hdc, int index) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_device_caps) {
        return 0;
    }

    const auto original = g_display_metrics_hook->m_get_device_caps.call<int>(hdc, index);

    if (!should_override_device_cap(index)) {
        return original;
    }

    const auto res = get_spoof_resolution();
    if (!res) {
        return original;
    }

    if (const auto overridden = override_device_cap(index, res->first, res->second); overridden) {
        return *overridden;
    }

    return original;
}

HMONITOR WINAPI DisplayMetricsHook::onMonitorFromWindow(HWND hwnd, DWORD flags) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_monitor_from_window) {
        return nullptr;
    }

    return g_display_metrics_hook->m_monitor_from_window.call<HMONITOR>(hwnd, flags);
}

BOOL WINAPI DisplayMetricsHook::onGetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_monitor_info_w) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_get_monitor_info_w.call<BOOL>(hMonitor, lpmi);

    if (!ok || lpmi == nullptr) {
        return ok;
    }

    const auto res = get_spoof_resolution();
    if (!res) {
        return ok;
    }

    const auto left = lpmi->rcMonitor.left;
    const auto top = lpmi->rcMonitor.top;

    lpmi->rcMonitor.right = left + static_cast<LONG>(res->first);
    lpmi->rcMonitor.bottom = top + static_cast<LONG>(res->second);

    lpmi->rcWork = lpmi->rcMonitor;
    return ok;
}

BOOL WINAPI DisplayMetricsHook::onGetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_monitor_info_a) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_get_monitor_info_a.call<BOOL>(hMonitor, lpmi);

    if (!ok || lpmi == nullptr) {
        return ok;
    }

    const auto res = get_spoof_resolution();
    if (!res) {
        return ok;
    }

    const auto left = lpmi->rcMonitor.left;
    const auto top = lpmi->rcMonitor.top;

    lpmi->rcMonitor.right = left + static_cast<LONG>(res->first);
    lpmi->rcMonitor.bottom = top + static_cast<LONG>(res->second);

    lpmi->rcWork = lpmi->rcMonitor;
    return ok;
}

BOOL WINAPI DisplayMetricsHook::onEnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_enum_display_monitors) {
        return FALSE;
    }

    // We don't currently filter the monitor list, only the monitor bounds.
    return g_display_metrics_hook->m_enum_display_monitors.call<BOOL>(hdc, lprcClip, lpfnEnum, dwData);
}

BOOL WINAPI DisplayMetricsHook::onGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl) {
    std::lock_guard _{ g_display_metrics_mutex };

    if (g_display_metrics_hook == nullptr || !g_display_metrics_hook->m_get_window_placement) {
        return FALSE;
    }

    const auto ok = g_display_metrics_hook->m_get_window_placement.call<BOOL>(hWnd, lpwndpl);

    if (!ok || lpwndpl == nullptr || lpwndpl->length < sizeof(WINDOWPLACEMENT)) {
        return ok;
    }

    const auto res = get_spoof_resolution_for_hwnd(hWnd);
    if (!res) {
        return ok;
    }

    // Preserve position, rewrite the "normal" rectangle size.
    const LONG left = lpwndpl->rcNormalPosition.left;
    const LONG top = lpwndpl->rcNormalPosition.top;

    lpwndpl->rcNormalPosition.right = left + static_cast<LONG>(res->first);
    lpwndpl->rcNormalPosition.bottom = top + static_cast<LONG>(res->second);

    return ok;
}
