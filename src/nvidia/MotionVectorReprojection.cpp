#include "MotionVectorReprojection.h"
#include <../../../_deps/directxtk12-src/Src/d3dx12.h>
#include <aer/ConstantsPool.h>
#include <d3d12.h>
#include <mods/VR.hpp>
#ifdef COMPILE_SHADERS
namespace shaders::compute {
    #include "motion_vector_correction_cs.h"
}
#endif
namespace
{

    struct MotionVectorCorrectionConstants
    {
        glm::mat4 undoCameraMotion{};
        glm::mat4 cameraMotionCorrection{};
        glm::vec4 texSize{};
        glm::vec2 mvecScale{0.5f, -0.5f};
        glm::vec2 pad;
    };
    //TODO it looks like there is no clear requirements on alignment for compute root signature, this probably overkill
    //TODO none of resources actually tells that it has to be 16 byes aligned for root Signature
    static_assert(sizeof(MotionVectorCorrectionConstants) % 16 == 0, "YourStruct size must be a multiple of 16 bytes");
    constexpr int CONSTANTS_COUNT = sizeof(MotionVectorCorrectionConstants) / 4;
}
bool MotionVectorReprojection::CreateComputeRootSignature(ID3D12Device* device)
{
    CD3DX12_DESCRIPTOR_RANGE1 srvRange;
    CD3DX12_DESCRIPTOR_RANGE1 uavRange;
    CD3DX12_ROOT_PARAMETER1 rootParams[3];

    // SRV descriptor table
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, d3d12::SRV_UAV_COUNT, 0, 0); // 3 SRVs starting at slot 0
    rootParams[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);

    // UAV descriptor table
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0); // 1 UAV at slot 0
    rootParams[1].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

    // Constants
    rootParams[2].InitAsConstants(CONSTANTS_COUNT, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, 
                                                       D3D_ROOT_SIGNATURE_VERSION_1_1, 
                                                       signature.GetAddressOf(), 
                                                       error.GetAddressOf());
    if (FAILED(hr)) {
        if (error)
            spdlog::error("Error: D3DX12SerializeVersionedRootSignature for compute shader: {}", 
                         static_cast<char*>(error->GetBufferPointer()));
        return false;
    }

    hr = device->CreateRootSignature(0, 
                                    signature->GetBufferPointer(), 
                                    signature->GetBufferSize(), 
                                    IID_PPV_ARGS(m_computeRootSignature.GetAddressOf()));
    if (FAILED(hr)) {
        spdlog::error("Error: CreateRootSignature for compute shader. HRESULT: {}", hr);
        return false;
    }
    
    spdlog::info("[VR] Compute shader root signature created.");
    return true;
}

void MotionVectorReprojection::ProcessMotionVectors(ID3D12Resource* motionVector, D3D12_RESOURCE_STATES mv_state, ID3D12Resource* depth1, D3D12_RESOURCE_STATES depth_state, uint32_t frame, ID3D12GraphicsCommandList* cmd_list)
{
    auto device = g_framework->get_d3d12_hook()->get_device();
    if (!device) {
        return;
    }

    D3D12_RESOURCE_BARRIER barriers[3] = {
        CD3DX12_RESOURCE_BARRIER::Transition(depth1, depth_state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(motionVector, mv_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::UAV(motionVector)
    };
    cmd_list->ResourceBarrier(3, barriers);

    cmd_list->SetComputeRootSignature(m_computeRootSignature.Get());
    cmd_list->SetPipelineState(m_compute_pso.Get());

    auto& motionVectorResource = m_small_ring_pool.getUavSlot(motionVector);
    
    ID3D12DescriptorHeap* heaps[] = { m_small_ring_pool.m_compute_heap->Heap()};
    cmd_list->SetDescriptorHeaps(1, heaps);
    cmd_list->SetComputeRootDescriptorTable(0, m_small_ring_pool.GetGpuSrvHandle(depth1));
    cmd_list->SetComputeRootDescriptorTable(1, m_small_ring_pool.m_compute_heap->GetGpuHandle(motionVectorResource.heapIndex));

    MotionVectorCorrectionConstants constants{};
    constants.texSize.x = static_cast<float>(motionVectorResource.Width);
    constants.texSize.y = static_cast<float>(motionVectorResource.Height);
    constants.texSize.z = 1.0f /  static_cast<float>(motionVectorResource.Width);
    constants.texSize.w = 1.0f /  static_cast<float>(motionVectorResource.Height);
    constants.mvecScale = m_mvecScale;

    constants.undoCameraMotion = GlobalPool::get_correction_matrix((int)frame, ((int)frame - 1));
    constants.cameraMotionCorrection = GlobalPool::get_correction_matrix((int)frame, ((int)frame - 2));
    cmd_list->SetComputeRoot32BitConstants(2, CONSTANTS_COUNT, &constants, 0);

    const UINT width = motionVectorResource.Width;
    const UINT height = motionVectorResource.Height;
    
    cmd_list->Dispatch((width + 15) / 16, (height + 15) / 16, 1);
    
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = depth_state;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = mv_state;
    cmd_list->ResourceBarrier(3, barriers);
}


bool MotionVectorReprojection::CreatePipelineStates(ID3D12Device* device, DXGI_FORMAT backBufferFormat)
{
    // Compute shader PSO
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC compute_pso = {};
        compute_pso.pRootSignature = m_computeRootSignature.Get();
#ifdef COMPILE_SHADERS
        compute_pso.CS = CD3DX12_SHADER_BYTECODE(&shaders::compute::g_CSMain, sizeof(shaders::compute::g_CSMain));
#endif
        compute_pso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        HRESULT hr = device->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(m_compute_pso.GetAddressOf()));
        if (FAILED(hr)) {
            spdlog::error("[VR] Failed to create compute pipeline state. HRESULT: {}", hr);
            return false;
        }
        spdlog::info("[VR] Compute shader PSO created.");
    }

    return true;
}

void MotionVectorReprojection::on_d3d12_initialize(ID3D12Device* device, const D3D12_RESOURCE_DESC& backBuffer_desc)
{

    m_small_ring_pool.Init(device);
    if (!CreateComputeRootSignature(device) ||
        !CreatePipelineStates(device, DXGI_FORMAT_R10G10B10A2_UNORM))
    {
        spdlog::error("Failed to setup MotionVectorFix");
        return;
    }

    m_initialized = true;
}

void MotionVectorReprojection::on_device_reset()
{
    Reset();
}