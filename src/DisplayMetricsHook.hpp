#pragma once

#include <optional>
#include <utility>

#include <Windows.h>
#include <safetyhook/inline_hook.hpp>

// Process-wide hooks for APIs games use to pick internal render resolution.
// IMPORTANT: This is intentionally Win32/GDI only (no DXGI/D3D).
class DisplayMetricsHook {
public:
    DisplayMetricsHook();
    DisplayMetricsHook(const DisplayMetricsHook&) = delete;
    DisplayMetricsHook(DisplayMetricsHook&&) = delete;
    ~DisplayMetricsHook();

    DisplayMetricsHook& operator=(const DisplayMetricsHook&) = delete;
    DisplayMetricsHook& operator=(DisplayMetricsHook&&) = delete;

    bool is_valid() const { return m_valid; }

private:
    bool m_valid{ false };

    safetyhook::InlineHook m_get_system_metrics{};
    safetyhook::InlineHook m_get_system_metrics_for_dpi{};
    safetyhook::InlineHook m_get_dpi_for_window{};
    safetyhook::InlineHook m_get_dpi_for_system{};

    safetyhook::InlineHook m_enum_display_settings_w{};
    safetyhook::InlineHook m_enum_display_settings_ex_w{};
    safetyhook::InlineHook m_enum_display_settings_a{};
    safetyhook::InlineHook m_enum_display_settings_ex_a{};

    safetyhook::InlineHook m_get_device_caps{};

    safetyhook::InlineHook m_monitor_from_window{};
    safetyhook::InlineHook m_get_monitor_info_w{};
    safetyhook::InlineHook m_get_monitor_info_a{};
    safetyhook::InlineHook m_enum_display_monitors{};

    safetyhook::InlineHook m_get_window_placement{};

private:
    static int WINAPI onGetSystemMetrics(int nIndex);
    static int WINAPI onGetSystemMetricsForDpi(int nIndex, UINT dpi);
    static UINT WINAPI onGetDpiForWindow(HWND hwnd);
    static UINT WINAPI onGetDpiForSystem();

    static BOOL WINAPI onEnumDisplaySettingsW(LPCWSTR deviceName, DWORD iModeNum, DEVMODEW* devMode);
    static BOOL WINAPI onEnumDisplaySettingsExW(LPCWSTR deviceName, DWORD iModeNum, DEVMODEW* devMode, DWORD flags);
    static BOOL WINAPI onEnumDisplaySettingsA(LPCSTR deviceName, DWORD iModeNum, DEVMODEA* devMode);
    static BOOL WINAPI onEnumDisplaySettingsExA(LPCSTR deviceName, DWORD iModeNum, DEVMODEA* devMode, DWORD flags);

    static int WINAPI onGetDeviceCaps(HDC hdc, int index);

    static HMONITOR WINAPI onMonitorFromWindow(HWND hwnd, DWORD flags);
    static BOOL WINAPI onGetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi);
    static BOOL WINAPI onGetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi);
    static BOOL WINAPI onEnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);

    static BOOL WINAPI onGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl);

private:
    static std::optional<std::pair<uint32_t, uint32_t>> get_spoof_resolution();
    static std::optional<std::pair<uint32_t, uint32_t>> get_spoof_resolution_for_hwnd(HWND hwnd);
    static bool is_target_game_window(HWND hwnd);
    static bool is_game_caller();

};

