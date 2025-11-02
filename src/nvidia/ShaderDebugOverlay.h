#pragma once

#include <DescriptorHeap.h>
#include "MotionVectorReprojection.h"
#include <d3d12.h>
#include <mods/vr/d3d12/ComPtr.hpp>
#include <mods/vr/d3d12/CommandContext.hpp>
#include <sl.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;


class ShaderDebugOverlay
{
public:
    inline static std::shared_ptr<ShaderDebugOverlay> &Get() {
        static auto instance = std::make_shared<ShaderDebugOverlay>();
        return instance;
    }

    void Draw(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *pResource, D3D12_CPU_DESCRIPTOR_HANDLE *rtv_handle);
    bool setup(ID3D12Device *device, D3D12_RESOURCE_DESC backBuffer_desc);

    inline void Reset() {
        m_debug_layer_pso.Reset();
        m_rootSignature.Reset();
        m_debug_heap.reset();
    }

    ShaderDebugOverlay() = default;

    ~ShaderDebugOverlay() {
        Reset();
    };

    static inline DXGI_FORMAT getCorrectDXGIFormat(DXGI_FORMAT Format)
    {
        switch (Format)
        {
        case DXGI_FORMAT_D16_UNORM: // casting from non typeless is supported from RS2+
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D32_FLOAT: // casting from non typeless is supported from RS2+
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32_TYPELESS:
            return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R16G16_TYPELESS:
            return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: // casting from non typeless is supported from RS2+
        case DXGI_FORMAT_R24G8_TYPELESS:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: // casting from non typeless is supported from RS2+
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:
            return Format;
        };
    }

private:
    enum SRV_HEAP : unsigned int {
        MVEC = 0,
        DEPTH = MVEC + 1,
        MVEC_PROCESSED = DEPTH + 1,
        COUNT = MVEC_PROCESSED + 1
    };

    bool CreateRootSignature(ID3D12Device *device);
    bool CreatePipelineStates(ID3D12Device *device, DXGI_FORMAT backBufferFormat);

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_debug_layer_pso;
    std::unique_ptr<DirectX::DescriptorHeap> m_debug_heap{};

};
