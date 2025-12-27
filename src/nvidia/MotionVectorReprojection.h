#pragma once
#include <DescriptorHeap.h>
#include <Framework.hpp>
#include <d3d12.h>
#include <mods/vr/d3d12/ComPtr.hpp>
#include "sl.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
//using namespace DirectX::SimpleMath;

namespace d3d12
{
    static DXGI_FORMAT getCorrectDXGIFormat(DXGI_FORMAT Format)
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

    static D3D12_SHADER_RESOURCE_VIEW_DESC getSRVdesc(const D3D12_RESOURCE_DESC& desc) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = getCorrectDXGIFormat(desc.Format);
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0; // Start viewing from the highest resolution mip
        return srvDesc;
    }
    constexpr int MAX_SMALL_RING_POOL_SIZE = 5;
    enum SRV_UAV : unsigned int
    {
        SRV_HEAP_START = 0,
        UAV_HEAP_START = SRV_HEAP_START + MAX_SMALL_RING_POOL_SIZE,
        SRV_UAV_COUNT  = UAV_HEAP_START + MAX_SMALL_RING_POOL_SIZE
    };
    struct RingResource
    {
        uintptr_t resource_ptr{};
        int heapIndex{-1};
        unsigned int Width{};
        unsigned int Height{};

        void Reset()
        {
            resource_ptr = 0;
            heapIndex = -1;
            Width = 0;
            Height = 0;
        }
    };
    struct SmallRingPool
    {
        RingResource srv_pool[MAX_SMALL_RING_POOL_SIZE]{};
        int current_srv_pool_index{0};
        RingResource uav_pool[MAX_SMALL_RING_POOL_SIZE]{};
        int current_uav_pool_index{0};
        std::unique_ptr<DescriptorHeap> m_compute_heap{};
        ID3D12Device* m_pDevice{nullptr};

        auto& GetSRVSlot(ID3D12Resource* pDepth)
        {
            auto ptr = reinterpret_cast<uintptr_t>(pDepth);
            for (auto &ring_resource : srv_pool) {
                if (ring_resource.heapIndex > 0 && ring_resource.resource_ptr == ptr) {
                    return ring_resource;
                }
            }
            current_srv_pool_index = (current_srv_pool_index + 1) % MAX_SMALL_RING_POOL_SIZE;
            int index = current_srv_pool_index;
            srv_pool[index].resource_ptr = ptr;
            srv_pool[index].heapIndex = index + (int)SRV_HEAP_START;
            auto desc = pDepth->GetDesc();
            srv_pool[index].Width = static_cast<unsigned int>(desc.Width);
            srv_pool[index].Height = desc.Height;
            auto srv_desc = getSRVdesc(desc);
            m_pDevice->CreateShaderResourceView(pDepth, &srv_desc, m_compute_heap->GetCpuHandle(srv_pool[index].heapIndex));
            return srv_pool[index];
        }

        auto& getUavSlot(ID3D12Resource* pMVec)
        {
            const auto ptr = reinterpret_cast<uintptr_t>(pMVec);
            for (auto &ring_resource : uav_pool) {
                if (ring_resource.heapIndex > 0 && ring_resource.resource_ptr == ptr) {
                    return ring_resource;
                }
            }
            current_uav_pool_index = (current_uav_pool_index + 1) % MAX_SMALL_RING_POOL_SIZE;
            int index = current_uav_pool_index;
            uav_pool[index].resource_ptr = ptr;
            uav_pool[index].heapIndex = index + (int)UAV_HEAP_START;
            auto desc = pMVec->GetDesc();
            uav_pool[index].Width = static_cast<unsigned int>(desc.Width);
            uav_pool[index].Height = desc.Height;
            m_pDevice->CreateUnorderedAccessView(
                    pMVec,
                    nullptr,
                    nullptr, m_compute_heap->GetCpuHandle(uav_pool[index].heapIndex)
            );
            return uav_pool[index];
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvHandle(ID3D12Resource* pResource)
        {
            RingResource& slot = GetSRVSlot(pResource);
            return m_compute_heap->GetGpuHandle(slot.heapIndex);
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuUavHandle(ID3D12Resource* pResource)
        {
            auto& slot = getUavSlot(pResource);
            return m_compute_heap->GetGpuHandle(slot.heapIndex);
        }

        void Init(ID3D12Device* pDevice)
        {
            try {
                m_compute_heap = std::make_unique<DescriptorHeap>(pDevice,
                                                                           D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                           D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                                                           SRV_UAV_COUNT);
                m_pDevice = pDevice;
            } catch(...) {
                spdlog::error("Failed to create SRV/RTV descriptor heap for MotionVectorFix");
            }
        }

        void Reset()
        {
            for (auto& res : srv_pool) {
                res.Reset();
            }
            current_srv_pool_index = 0;
            for (auto& res : uav_pool) {
                res.Reset();
            }
            current_uav_pool_index = 0;
            m_compute_heap.reset();
            m_pDevice = nullptr;

        }


    };
}

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

    void Reset()
    {
        m_initialized = false;
        m_compute_pso.Reset();
        m_computeRootSignature.Reset();
        m_small_ring_pool.Reset();
    }

    // Run the compute shader to process motion vectors
    //    void ProcessMotionVectors(ID3D12Resource* mvec, D3D12_RESOURCE_STATES state, uint32_t frame);
    void ProcessMotionVectors(ID3D12Resource* mvec, D3D12_RESOURCE_STATES mvec_state, ID3D12Resource* depth, D3D12_RESOURCE_STATES depth_state, uint32_t frame,
                              ID3D12GraphicsCommandList* cmd_list);

    glm::vec2 m_mvecScale {0.5f, -0.5f};
private:

    bool CreateComputeRootSignature(ID3D12Device* device);
    bool CreatePipelineStates(ID3D12Device* device, DXGI_FORMAT backBufferFormat);

    ComPtr<ID3D12RootSignature>              m_computeRootSignature;
    ComPtr<ID3D12PipelineState>              m_compute_pso;
    d3d12::SmallRingPool                     m_small_ring_pool;
    bool m_initialized{ false };
};
