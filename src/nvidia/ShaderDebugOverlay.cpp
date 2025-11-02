#include "ShaderDebugOverlay.h"
#include <../../../directxtk12-src/Src/PlatformHelpers.h>
#include <../../../_deps/directxtk12-src/Src/d3dx12.h>
#include <ModSettings.h>
#include <d3d12.h>
#include <mods/VR.hpp>

#ifdef COMPILE_SHADERS
namespace shaders::debug {
    #include "debug_layer_ps.h"
    #include "debug_layer_vs.h"
}

#endif

struct ShaderConstants {
    int width;
    int height;
    uint32_t frame_count;
    int apply_fix;
};

ShaderConstants g_shader_constants{};

bool ShaderDebugOverlay::CreateRootSignature(ID3D12Device* device)
{
    HRESULT                 hr;
    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1];
    CD3DX12_ROOT_PARAMETER1   rootParams[2];

    descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_HEAP::COUNT, 0, 0);
    rootParams[0].InitAsDescriptorTable(1, &descriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsConstants(4, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    CD3DX12_STATIC_SAMPLER_DESC staticSamplers[2];

    // D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT
    staticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // D3D12_FILTER_MIN_MAG_MIP_POINT
    staticSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams,
                               _countof(staticSamplers), staticSamplers, rootSignatureFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, signature.GetAddressOf(), error.GetAddressOf());
    if (FAILED(hr)) {
        if (error)
            spdlog::error("Error: D3DX12SerializeVersionedRootSignature {}", static_cast<char*>(error->GetBufferPointer()));
        return false;
    }
    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf()));
    if (FAILED(hr)) {
        if (error)
            spdlog::error("Error: CreateRootSignature {}", static_cast<char*>(error->GetBufferPointer()));
        return false;
    }
    spdlog::info("[VR] Debug Layer root signature created.");
    return true;
}

bool ShaderDebugOverlay::CreatePipelineStates(ID3D12Device* device, DXGI_FORMAT backBufferFormat)
{
    // Debug layer PSO
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC debug_layer_pso = {};
        debug_layer_pso.InputLayout = { };
        debug_layer_pso.pRootSignature = m_rootSignature.Get();
#ifdef COMPILE_SHADERS
        debug_layer_pso.VS = CD3DX12_SHADER_BYTECODE(&shaders::debug::g_vs_main, sizeof(shaders::debug::g_vs_main));
        debug_layer_pso.PS = CD3DX12_SHADER_BYTECODE(&shaders::debug::g_ps_main, sizeof(shaders::debug::g_ps_main));
#endif
        auto blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

        debug_layer_pso.BlendState = blendDesc;
        debug_layer_pso.DepthStencilState             = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        debug_layer_pso.DepthStencilState.DepthEnable = false;
        debug_layer_pso.SampleMask                    = UINT_MAX;
        debug_layer_pso.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
        debug_layer_pso.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        debug_layer_pso.NumRenderTargets              = 1;
        debug_layer_pso.DSVFormat                     = DXGI_FORMAT_UNKNOWN;
        debug_layer_pso.RTVFormats[0]                 = backBufferFormat;
        debug_layer_pso.SampleDesc.Count              = 1;
        debug_layer_pso.SampleDesc.Quality            = 0;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&debug_layer_pso, IID_PPV_ARGS(m_debug_layer_pso.GetAddressOf())));
        spdlog::info("[VR] Debug layer PSO created.");
    }
    
    return true;
}

D3D12_SHADER_RESOURCE_VIEW_DESC getSRVdesc1(const D3D12_RESOURCE_DESC& desc) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = ShaderDebugOverlay::getCorrectDXGIFormat(desc.Format);
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0; // Start viewing from the highest resolution mip
    return srvDesc;
}

void ShaderDebugOverlay::Draw(ID3D12GraphicsCommandList* commandList, ID3D12Resource* backbuffer, D3D12_CPU_DESCRIPTOR_HANDLE* rtv_handle) {
    static auto mvReprojection = MotionVectorReprojection::Get();
    if(!ModSettings::g_internalSettings.debugShaders || !mvReprojection->isValid()) {
        return;
    }

    auto device = g_framework->get_d3d12_hook()->get_device();
    if (!device) {
        return;
    }

//    mvReprojection->commandContext.wait(10);

    const auto render_target_desc = backbuffer->GetDesc();

    {
        D3D12_RESOURCE_BARRIER barriers[5]
        {
            CD3DX12_RESOURCE_BARRIER::Transition(backbuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
//            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[3].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[0].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[1].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[2].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[3].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
//            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_depth_buffer[0].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
//            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_depth_buffer[1].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        };
        commandList->ResourceBarrier(5, barriers);
    }

    float clear_color[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

//    const auto& depthDesc      = mvReprojection->m_depth_buffer[0]->GetDesc();
//    auto depth_srv_desc = getSRVdesc1(depthDesc);
    static auto vr = VR::get();
    const auto& mvectorDesc = mvReprojection->m_motion_vector_buffer[vr->m_presenter_frame_count % 4]->GetDesc();
    auto mvector_srv_desc = getSRVdesc1(mvectorDesc);

    device->CreateShaderResourceView(mvReprojection->m_motion_vector_buffer[(vr->m_presenter_frame_count)% 4].Get(), &mvector_srv_desc, m_debug_heap->GetCpuHandle(SRV_HEAP::MVEC));
//    device->CreateShaderResourceView(mvReprojection->m_depth_buffer[0].Get(), &depth_srv_desc, m_debug_heap->GetCpuHandle(SRV_HEAP::DEPTH));
//    device->CreateShaderResourceView(mvReprojection->m_motion_vector_buffer[2].Get(), &mvector_srv_desc, m_debug_heap->GetCpuHandle(SRV_HEAP::MVEC_PROCESSED));

    D3D12_VIEWPORT viewport{};
    viewport.Width    = (float)render_target_desc.Width/2;
    viewport.Height   = (float)render_target_desc.Height;
    viewport.MinDepth = D3D12_MIN_DEPTH;
    viewport.MaxDepth = D3D12_MAX_DEPTH;

    D3D12_RECT scissor_rect{};
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = (LONG)render_target_desc.Width/2;
    scissor_rect.bottom = (LONG)render_target_desc.Height;


    auto constants = g_shader_constants;

    auto rpc_window_size
        = XMFLOAT4(1.0f / ((float)render_target_desc.Width), 1.0f / ((float)render_target_desc.Height),
                   ((float)render_target_desc.Width), ((float)render_target_desc.Height));


    commandList->SetPipelineState(m_debug_layer_pso.Get());
//    commandList->ClearRenderTargetView(*rtv_handle, clear_color, 1, &scissor_rect);
    commandList->OMSetRenderTargets(1, rtv_handle, FALSE, nullptr);

    // Clear the render target within the scissor rectangle
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor_rect);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->SetGraphicsRoot32BitConstants(1, 4, &g_shader_constants, 0);

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_debug_heap->Heap()
    };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);
    commandList->SetGraphicsRootDescriptorTable(0, m_debug_heap->GetFirstGpuHandle());

    // Draw the quad
    commandList->DrawInstanced(6, 1, 0, 0);

    {
        D3D12_RESOURCE_BARRIER barriers[5]
        {
            CD3DX12_RESOURCE_BARRIER::Transition(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
//            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[3].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[0].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[1].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[2].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_motion_vector_buffer[3].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
//            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_depth_buffer[0].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
//            CD3DX12_RESOURCE_BARRIER::Transition(mvReprojection->m_depth_buffer[1].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ)
        };
        commandList->ResourceBarrier(5, barriers);
    }
}

bool ShaderDebugOverlay::setup(ID3D12Device* device, D3D12_RESOURCE_DESC backBuffer_desc)
{
    try {
        m_debug_heap = std::make_unique<DirectX::DescriptorHeap>(device,
                                                                   D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, SRV_HEAP::COUNT);

    } catch(...) {
        spdlog::error("Failed to create SRV/RTV descriptor heap for MotionVectorFix");
        return false;
    }

    if (!CreateRootSignature(device) ||
        !CreatePipelineStates(device, DXGI_FORMAT_R10G10B10A2_UNORM))
    {
        spdlog::error("Failed to setup MotionVectorFix");
        return false;
    }
    return true;
}
