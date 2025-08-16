#pragma once
#ifdef DXGI_INJECTION
//#include "D3D12Hook.hpp"
#include <Dxgi1_3.h>
#include <dxgi.h>
//#include <interceptors/D3D12DeviceHook.h>
//#include <interceptors/DXGIFactory.h>
//#include <interceptors/PixInjector.h>
#include <windows.h>

HMODULE g_dxgi = 0;
HMODULE g_d3d12 = 0;

bool load_dxgi()
{
    if (g_dxgi) {
        return true;
    }

    wchar_t buffer[MAX_PATH]{ 0 };
    if (GetSystemDirectoryW(buffer, MAX_PATH) != 0) {
        // Load the original dinput8.dll
        if ((g_dxgi = LoadLibraryW((std::wstring{ buffer } + L"\\dxgi.dll").c_str())) == NULL) {
            spdlog::error("Failed to load dxgi.dll");
            return false;
        }

        return true;
    }
    return false;
}

typedef HRESULT(WINAPI* ID3D12Device_CreateDevice)(_In_opt_ IUnknown*, D3D_FEATURE_LEVEL, _In_ REFIID, _COM_Outptr_opt_ void**);
ID3D12Device_CreateDevice pCreateD3DDevice = nullptr; //original function pointer after hook
ID3D12Device_CreateDevice pCreateD3DDeviceOrig = nullptr; //original function pointer after hook


HRESULT WINAPI d3d12_CreateDevice(_In_opt_ IUnknown* pAdapter,
                                  D3D_FEATURE_LEVEL MinimumFeatureLevel,
                                  _In_ REFIID riid, // Expected: ID3D12Device
                                  _COM_Outptr_opt_ void** ppDevice) {
    auto result = pCreateD3DDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);
    //hook detourCreateCommittedResource from ppDevice
    if (SUCCEEDED(result)) {
        spdlog::info("ID3D12Device created hooking");
//        ID3D12Device_Hook(riid, ppDevice);
    }
    return result;
}

//bool hook_d3d12()
//{
//    if(g_d3d12)
//    {
//        return true;
//    }
//    while ((g_d3d12 = GetModuleHandle("d3d12")) == NULL) {
//        Sleep(50);
//    }
//    auto status = MH_CreateHookApiEx(L"d3d12", "D3D12CreateDevice", &d3d12_CreateDevice, reinterpret_cast<void**>(&pCreateD3DDevice), reinterpret_cast<void**>(&pCreateD3DDeviceOrig));
//    if (status != MH_OK) {
//        spdlog::error("Failed to hook D3D12CreateDevice: {}", status);
//        return false;
//    }
//
//    status = MH_EnableHook(pCreateD3DDeviceOrig);
//
//    if (status != MH_OK) {
//        spdlog::error("Failed to hook D3D12CreateDevice: {}", status);
//        return false;
//    }
//    spdlog::info("Hooked D3D12CreateDevice");
//    return true;
//}

extern "C"
{
    // CreateDXGIFactory wrapper for dxgi.dll
    __declspec(dllexport) HRESULT WINAPI dxgi_create_factory(REFIID riid, _COM_Outptr_ void** ppFactory)
    {
        // This needs to be done because when we include dinput.h in DInputHook,
        // It is a redefinition, so we assign an export by not using the original name
#pragma comment(linker, "/EXPORT:CreateDXGIFactory=dxgi_create_factory")

        load_dxgi();
        static auto original_fn =  ((decltype(CreateDXGIFactory)*)GetProcAddress(g_dxgi, "CreateDXGIFactory"));
        auto result =  original_fn(riid, ppFactory);
        spdlog::debug("CreateDXGIFactory called from proxy");
        /*if (SUCCEEDED(result)) {
            IDXGIFactory1* pFactory = reinterpret_cast<IDXGIFactory1*>(*ppFactory);
            IDXGIFactory1_Hook(pFactory);
        }*/
        return result;
    }
}

extern "C"
{
    // CreateDXGIFactory1 wrapper for dxgi.dll
    __declspec(dllexport) HRESULT WINAPI dxgi_create_factory1(REFIID riid, _COM_Outptr_ void** ppFactory)
    {
        // This needs to be done because when we include dinput.h in DInputHook,
        // It is a redefinition, so we assign an export by not using the original name
#pragma comment(linker, "/EXPORT:CreateDXGIFactory1=dxgi_create_factory1")

        load_dxgi();
        static auto original_fn = ((decltype(CreateDXGIFactory1)*)GetProcAddress(g_dxgi, "CreateDXGIFactory1"));
        auto result = original_fn(riid, ppFactory);
        spdlog::debug("CreateDXGIFactory1 called from proxy");
        /*if (SUCCEEDED(result)) {
            IDXGIFactory1* pFactory1 = reinterpret_cast<IDXGIFactory1*>(*ppFactory);
            IDXGIFactory1_Hook(pFactory1);
        }*/
        return result;
    }
}

extern "C"
{
    // CreateDXGIFactory2 wrapper for dxgi.dll
    __declspec(dllexport) HRESULT WINAPI dxgi_create_factory2(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory)
    {
        // This needs to be done because when we include dinput.h in DInputHook,
        // It is a redefinition, so we assign an export by not using the original name
#pragma comment(linker, "/EXPORT:CreateDXGIFactory2=dxgi_create_factory2")

        load_dxgi();
//        WindowsDebug::init();
        auto static original_fn = ((decltype(CreateDXGIFactory2)*)GetProcAddress(g_dxgi, "CreateDXGIFactory2"));
        auto result = original_fn(Flags, riid, ppFactory);
        spdlog::debug("CreateDXGIFactory2 called from proxy");
        /*if (SUCCEEDED(result)) {
             D3D12Hook::hook_swapchain(reinterpret_cast<IDXGIFactory2*>(*ppFactory));
            IDXGIFactory2** pFactory2 = reinterpret_cast<IDXGIFactory2**>(ppFactory);
            IDXGIFactory2_Hook(*pFactory2);
        }*/
        return result;
    }
}

extern "C"
{
    // DXGIGetDebugInterface1 wrapper for dxgi.dll
    __declspec(dllexport) HRESULT WINAPI dxgi_get_debug_interface(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory)
    {
        // This needs to be done because when we include dinput.h in DInputHook,
        // It is a redefinition, so we assign an export by not using the original name
#pragma comment(linker, "/EXPORT:DXGIGetDebugInterface1=dxgi_get_debug_interface")

        load_dxgi();
        static auto original_fn = ((decltype(DXGIGetDebugInterface1)*)GetProcAddress(g_dxgi, "DXGIGetDebugInterface1"));
        return original_fn(Flags, riid, ppFactory);
    }
}
#endif
