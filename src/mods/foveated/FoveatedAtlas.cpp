#include "FoveatedAtlas.hpp"

namespace foveated {

FoveatedAtlas& FoveatedAtlas::get() {
    static FoveatedAtlas inst;
    return inst;
}

bool FoveatedAtlas::initialize(ID3D12Device* dev, const AtlasConfig& cfg) {
    if (dev == nullptr) {
        return false;
    }

    m_cfg = cfg;
    const auto totalWidth = getTotalWidth();
    const auto totalHeight = getTotalHeight();

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc{};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = totalWidth;
    texDesc.Height = totalHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = m_cfg.format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clear{};
    clear.Format = m_cfg.format;
    clear.Color[0] = 0.0f;
    clear.Color[1] = 0.0f;
    clear.Color[2] = 0.0f;
    clear.Color[3] = 1.0f;

    if (FAILED(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&m_atlas)))) {
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.NumDescriptors = 1;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    if (FAILED(dev->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        shutdown();
        return false;
    }

    m_rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    dev->CreateRenderTargetView(m_atlas.Get(), nullptr, m_rtv);

    // Depth buffer (optional)
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if (SUCCEEDED(dev->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsvHeap)))) {
        D3D12_RESOURCE_DESC depthDesc = texDesc;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClear{};
        depthClear.Format = DXGI_FORMAT_D32_FLOAT;
        depthClear.DepthStencil.Depth = 1.0f;
        depthClear.DepthStencil.Stencil = 0;

        if (SUCCEEDED(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
                                                   D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&m_depth)))) {
            m_dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
            dev->CreateDepthStencilView(m_depth.Get(), nullptr, m_dsv);
        }
    }

    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    return true;
}

void FoveatedAtlas::shutdown() {
    m_atlas.Reset();
    m_depth.Reset();
    m_rtvHeap.Reset();
    m_dsvHeap.Reset();
    m_rtv = {};
    m_dsv = {};
    m_state = D3D12_RESOURCE_STATE_COMMON;
}

void FoveatedAtlas::transitionToRT(ID3D12GraphicsCommandList* cmd) {
    if (!m_atlas || cmd == nullptr || m_state == D3D12_RESOURCE_STATE_RENDER_TARGET) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_atlas.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = m_state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    cmd->ResourceBarrier(1, &barrier);
    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void FoveatedAtlas::transitionToSRV(ID3D12GraphicsCommandList* cmd) {
    if (!m_atlas || cmd == nullptr || m_state == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_atlas.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = m_state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    cmd->ResourceBarrier(1, &barrier);
    m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

} // namespace foveated
