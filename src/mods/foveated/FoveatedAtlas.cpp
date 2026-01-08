#include "FoveatedAtlas.hpp"
#include <spdlog/spdlog.h>

namespace foveated {

bool FoveatedAtlas::initialize(ID3D12Device* dev, const AtlasConfig& cfg) {
    if (!dev) {
        spdlog::error("[FoveatedAtlas] Cannot initialize with null device");
        return false;
    }
    
    m_cfg = cfg;
    
    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    HRESULT hr = dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr)) {
        spdlog::error("[FoveatedAtlas] Failed to create RTV descriptor heap: {:x}", hr);
        return false;
    }
    
    // Create DSV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    hr = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
    if (FAILED(hr)) {
        spdlog::error("[FoveatedAtlas] Failed to create DSV descriptor heap: {:x}", hr);
        return false;
    }
    
    // Create atlas render target (double-height for 4-view layout)
    D3D12_RESOURCE_DESC rtDesc{};
    rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rtDesc.Alignment = 0;
    rtDesc.Width = getTotalWidth();
    rtDesc.Height = getTotalHeight();
    rtDesc.DepthOrArraySize = 1;
    rtDesc.MipLevels = 1;
    rtDesc.Format = cfg.format;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.SampleDesc.Quality = 0;
    rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = cfg.format;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;
    
    hr = dev->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &rtDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue,
        IID_PPV_ARGS(&m_atlas)
    );
    
    if (FAILED(hr)) {
        spdlog::error("[FoveatedAtlas] Failed to create atlas resource: {:x}", hr);
        return false;
    }
    
    // Create depth stencil buffer
    D3D12_RESOURCE_DESC depthDesc{};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = getTotalWidth();
    depthDesc.Height = getTotalHeight();
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;
    
    hr = dev->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&m_depth)
    );
    
    if (FAILED(hr)) {
        spdlog::error("[FoveatedAtlas] Failed to create depth resource: {:x}", hr);
        return false;
    }
    
    // Create RTV
    m_rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    dev->CreateRenderTargetView(m_atlas.Get(), nullptr, m_rtv);
    
    // Create DSV
    m_dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    dev->CreateDepthStencilView(m_depth.Get(), nullptr, m_dsv);
    
    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    
    spdlog::info("[FoveatedAtlas] Initialized atlas {}x{} (foveal: {}x{}, peripheral: {}x{})",
        getTotalWidth(), getTotalHeight(),
        cfg.fovealWidth, cfg.fovealHeight,
        cfg.peripheralWidth, cfg.peripheralHeight);
    
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
    
    spdlog::info("[FoveatedAtlas] Shutdown complete");
}

void FoveatedAtlas::transitionToRT(ID3D12GraphicsCommandList* cmd) {
    if (!cmd || !m_atlas || m_state == D3D12_RESOURCE_STATE_RENDER_TARGET) {
        return;
    }
    
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_atlas.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = m_state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    
    cmd->ResourceBarrier(1, &barrier);
    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void FoveatedAtlas::transitionToSRV(ID3D12GraphicsCommandList* cmd) {
    if (!cmd || !m_atlas || m_state == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        return;
    }
    
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_atlas.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = m_state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    
    cmd->ResourceBarrier(1, &barrier);
    m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

} // namespace foveated
