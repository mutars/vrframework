#pragma once
#include <DescriptorHeap.h>
#include <Framework.hpp>
#include <d3d12.h>
#include <mods/vr/d3d12/ComPtr.hpp>
#include "sl.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
//using namespace DirectX::SimpleMath;

class MotionVectorReprojection
{
public:
    MotionVectorReprojection() = default;
    ~MotionVectorReprojection() { Reset(); };

    // Lifecycle methods (called by UpscalerAfrNvidiaModule)
    void on_d3d12_initialize(ID3D12Device* device, const D3D12_RESOURCE_DESC& backBuffer_desc);
    void on_device_reset();

    [[nodiscard]] inline bool isInitialized() const
    {
        if (!g_framework->is_ready() || !m_initialized) {
            return false;
        }
        return true;
    }

    inline void Reset()
    {
        m_initialized = false;
        m_compute_pso.Reset();
        m_computeRootSignature.Reset();
        m_compute_heap.reset();
        //        m_depth_buffer[0].Reset();
        //        m_depth_buffer[1].Reset();
        //        m_depth_buffer[2].Reset();
        //        m_depth_buffer[3].Reset();
    }

    // Run the compute shader to process motion vectors
    //    void ProcessMotionVectors(ID3D12Resource* mvec, D3D12_RESOURCE_STATES state, uint32_t frame);
    void ProcessMotionVectors(ID3D12Resource* mvec, D3D12_RESOURCE_STATES mvec_state, ID3D12Resource* depth, D3D12_RESOURCE_STATES depth_state, uint32_t frame,
                              ID3D12GraphicsCommandList* cmd_list);

    inline static DXGI_FORMAT getCorrectDXGIFormat(DXGI_FORMAT Format)
    {
        switch (Format) {
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
        }
    }

private:
    enum SRV_UAV : unsigned int
    {
        DEPTH_CURRENT = 0,
        MVEC_UAV      = DEPTH_CURRENT + 1,
        COUNT         = MVEC_UAV + 1
    };

    //    ComPtr<ID3D12Resource> m_depth_buffer[4]{};

    bool CreateComputeRootSignature(ID3D12Device* device);
    bool CreatePipelineStates(ID3D12Device* device, DXGI_FORMAT backBufferFormat);

    ComPtr<ID3D12RootSignature>              m_computeRootSignature;
    ComPtr<ID3D12PipelineState>              m_compute_pso;
    std::unique_ptr<DirectX::DescriptorHeap> m_compute_heap{};

    bool m_initialized{ false };
};
