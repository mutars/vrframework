#include "MotionVectorReprojection.h"
#include <../../../_deps/directxtk12-src/Src/d3dx12.h>
#include <ModSettings.h>
#include <aer/ConstantsPool.h>
#include <d3d12.h>
#include <mods/VR.hpp>

#ifdef COMPILE_SHADERS
namespace shaders::compute {
    #include "motion_vector_correction_cs.h"
}
#endif

struct alignas(4) MotionVectorCorrectionConstants {
    glm::mat4 correction{};
    glm::mat4 cameraMotionCorrection{};
    XMFLOAT4 texSize{};
};
static const int CONSTANTS_COUNT = sizeof(MotionVectorCorrectionConstants) / 4;

bool MotionVectorReprojection::ValidateResource(ID3D12Resource* source, ComPtr<ID3D12Resource> buffers[4])
{
    if (source == nullptr) {
        return false;
    }
    D3D12_RESOURCE_DESC desc = source->GetDesc();
    if (buffers[0] != nullptr) {
        D3D12_RESOURCE_DESC desc2 = buffers[0]->GetDesc();
        if (desc.Width != desc2.Width || desc.Height != desc2.Height || desc.Format != desc2.Format) {
            spdlog::info("Resource size mismatch {} {} {} {} {} {} {} {}", fmt::ptr(source), desc.Width, desc.Height, desc.Format, fmt::ptr(buffers[0].Get()), desc2.Width,
                         desc2.Height, desc2.Format);
            buffers[0].Reset();
            buffers[1].Reset();
            buffers[2].Reset();
            buffers[3].Reset();
        }
    }
    auto device = g_framework->get_d3d12_hook()->get_device();
    if (device == nullptr) {
        return false;
    }
    if (buffers[0] == nullptr || buffers[1] == nullptr || buffers[2] == nullptr || buffers[3] == nullptr) {
        D3D12_HEAP_PROPERTIES heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        spdlog::info("[VR] Creating resource copy for AFR motion vetor backbuffer. format={}", desc.Format);
        for (int i = 0; i < 4; i++) {
            HRESULT hr = device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                                IID_PPV_ARGS(buffers[i].GetAddressOf()));
            if (FAILED(hr))
            {
                spdlog::error("[VR] Failed to create resource {} HRESULT: 0x{:X}", desc.Format, static_cast<unsigned int>(hr));
                return false;
            }
        }
    }
    return true;
}

bool MotionVectorReprojection::CreateComputeRootSignature(ID3D12Device* device)
{
    CD3DX12_DESCRIPTOR_RANGE1 srvRange;
    CD3DX12_DESCRIPTOR_RANGE1 uavRange;
    CD3DX12_ROOT_PARAMETER1 rootParams[3];

    // SRV descriptor table
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_UAV::COUNT, 0, 0); // 3 SRVs starting at slot 0
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

D3D12_SHADER_RESOURCE_VIEW_DESC getSRVdesc(const D3D12_RESOURCE_DESC& desc) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = MotionVectorReprojection::getCorrectDXGIFormat(desc.Format);
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0; // Start viewing from the highest resolution mip
    return srvDesc;
}

void MotionVectorReprojection::ProcessMotionVectors(ID3D12Resource* motionVector, D3D12_RESOURCE_STATES mv_state, ID3D12Resource* depth1, D3D12_RESOURCE_STATES depth_state, uint32_t frame, ID3D12GraphicsCommandList* cmd_list)
{
    auto device = g_framework->get_d3d12_hook()->get_device();
    if (!device) {
        return;
    }

    // Transition resources to compute shader read/write states
    D3D12_RESOURCE_BARRIER barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(depth1, depth_state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(motionVector, mv_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmd_list->ResourceBarrier(2, barriers);

    // Set compute root signature and PSO
    cmd_list->SetComputeRootSignature(m_computeRootSignature.Get());
    cmd_list->SetPipelineState(m_compute_pso.Get());
    
    // Set heaps
    ID3D12DescriptorHeap* heaps[] = { m_compute_heap->Heap()};
    cmd_list->SetDescriptorHeaps(1, heaps);

    const auto& depthDesc      = depth1->GetDesc();
    auto depth_srv_desc = getSRVdesc(depthDesc);

    device->CreateShaderResourceView(depth1, &depth_srv_desc, m_compute_heap->GetCpuHandle(SRV_UAV::DEPTH_CURRENT));

    device->CreateUnorderedAccessView(
            motionVector,
            nullptr,
            nullptr, m_compute_heap->GetCpuHandle(SRV_UAV::MVEC_UAV)
    );

    // Set descriptor tables
    cmd_list->SetComputeRootDescriptorTable(0, m_compute_heap->GetFirstGpuHandle());
    cmd_list->SetComputeRootDescriptorTable(1, m_compute_heap->GetGpuHandle(SRV_UAV::MVEC_UAV));

    // Set constants
    MotionVectorCorrectionConstants constants{};
    auto desc = motionVector->GetDesc();
    constants.texSize.x = (float) desc.Width;
    constants.texSize.y = (float) desc.Height;
    constants.texSize.z = 1.0f /  (float) desc.Width;
    constants.texSize.w = 1.0f /  (float) desc.Height;

    static auto vr = VR::get();

    glm::mat4 correction = glm::mat4(1.0f);
    if(vr->is_hmd_active()) {
        auto eye = GlobalPool::get_eye_view(frame);
        auto past_eye = GlobalPool::get_eye_view(frame - 1);
        if(frame % 2 == 0) {
            auto hmd_transform = GlobalPool::get_hmd_view(frame);
            auto hmd_transform_past = GlobalPool::get_hmd_view(frame - 2);
            correction = eye * hmd_transform_past * glm::inverse(hmd_transform) * glm::inverse(past_eye);
        } else {
            correction = eye * glm::inverse(past_eye);
        }
    } else if(ModSettings::g_internalSettings.cameraShake) {
        correction[3].x = 0.068f * (frame % 2 != 0 ? -1.0f : +1.0f);
    }

    auto& sl_constants_n_1           = GlobalPool::getSlConstants(frame - 1);
    auto& sl_constants_n           = GlobalPool::getSlConstants(frame);
    auto projection = *(glm::mat4*) &sl_constants_n.cameraViewToClip;
    constants.correction = projection * correction * glm::inverse(projection);
    constants.cameraMotionCorrection = *(glm::mat4 *)&sl_constants_n_1.prevClipToClip;

    cmd_list->SetComputeRoot32BitConstants(2, CONSTANTS_COUNT, &constants, 0);

    // Get dimensions for dispatch
    UINT width = static_cast<UINT>(desc.Width);
    UINT height = static_cast<UINT>(desc.Height);
    
    // Dispatch compute shader
    cmd_list->Dispatch((width + 15) / 16, (height + 15) / 16, 1);
    
    // Transition back to original states
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = depth_state;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = mv_state;
    cmd_list->ResourceBarrier(2, barriers);
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
    try {
        m_compute_heap = std::make_unique<DirectX::DescriptorHeap>(device,
                                                                   D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                                                   SRV_UAV::COUNT);

    } catch(...) {
        spdlog::error("Failed to create SRV/RTV descriptor heap for MotionVectorFix");
        return;
    }
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

void MotionVectorReprojection::CopyResource(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* pSrcResource, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES srcState,
                                            D3D12_RESOURCE_STATES dstState)
{
    D3D12_RESOURCE_BARRIER barriers[2] = { CD3DX12_RESOURCE_BARRIER::Transition(pSrcResource, srcState, D3D12_RESOURCE_STATE_COPY_SOURCE),
                                           CD3DX12_RESOURCE_BARRIER::Transition(pDstResource, dstState, D3D12_RESOURCE_STATE_COPY_DEST) };
    cmdList->ResourceBarrier(2, barriers);
    cmdList->CopyResource(pDstResource, pSrcResource);
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.StateAfter  = srcState;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter  = dstState;
    cmdList->ResourceBarrier(2, barriers);
}
