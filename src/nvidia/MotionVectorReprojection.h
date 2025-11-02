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
    friend class UpscalerAfrNvidiaModule;
    friend class ShaderDebugOverlay;

    inline static std::shared_ptr<MotionVectorReprojection>& Get()
    {
        static auto instance = std::make_shared<MotionVectorReprojection>();
        return instance;
    }

    inline bool isInitialized() const
    {
        if (!g_framework->is_ready() || !initialized) {
            return false;
        }
        return true;
    }

    inline bool isValid()
    {
        for (const auto& buffer : m_motion_vector_buffer) {
            if (!buffer)
                return false;
        }
        //        for (const auto& buffer : m_depth_buffer) {
        //            if (!buffer) return false;
        //        }
        return true;
    }

    bool setup(ID3D12Device* device, D3D12_RESOURCE_DESC backBuffer_desc);

    inline void Reset()
    {
        initialized = false;
        m_compute_pso.Reset();
        m_computeRootSignature.Reset();
        m_compute_heap.reset();

        m_motion_vector_buffer[0].Reset();
        m_motion_vector_buffer[1].Reset();
        m_motion_vector_buffer[2].Reset();
        m_motion_vector_buffer[3].Reset();
        //        m_depth_buffer[0].Reset();
        //        m_depth_buffer[1].Reset();
        //        m_depth_buffer[2].Reset();
        //        m_depth_buffer[3].Reset();
    }

    // Run the compute shader to process motion vectors
    //    void ProcessMotionVectors(ID3D12Resource* mvec, D3D12_RESOURCE_STATES state, uint32_t frame);
    void ProcessMotionVectors(ID3D12Resource* mvec, D3D12_RESOURCE_STATES mvec_state, ID3D12Resource* depth, D3D12_RESOURCE_STATES depth_state, uint32_t frame,
                              ID3D12GraphicsCommandList* cmd_list);

    MotionVectorReprojection() = default;

    ~MotionVectorReprojection() { Reset(); };

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

    static void CopyResource(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* pSrcResource, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES srcState,
                                    D3D12_RESOURCE_STATES dstState);

private:
    enum SRV_UAV : unsigned int
    {
        DEPTH_CURRENT = 0,
        MVEC_UAV      = DEPTH_CURRENT + 1,
        COUNT         = MVEC_UAV + 1
    };

    ComPtr<ID3D12Resource> m_motion_vector_buffer[4]{};
    //    ComPtr<ID3D12Resource> m_depth_buffer[4]{};

    static bool ValidateResource(ID3D12Resource* source, ComPtr<ID3D12Resource> buffers[4]);

    bool CreateComputeRootSignature(ID3D12Device* device);
    bool CreatePipelineStates(ID3D12Device* device, DXGI_FORMAT backBufferFormat);

    ComPtr<ID3D12RootSignature>              m_computeRootSignature;
    ComPtr<ID3D12PipelineState>              m_compute_pso;
    std::unique_ptr<DirectX::DescriptorHeap> m_compute_heap{};

    bool initialized{ false };
};
