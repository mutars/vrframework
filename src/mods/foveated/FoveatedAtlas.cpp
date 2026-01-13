#include "FoveatedAtlas.hpp"
#include <spdlog/spdlog.h>

namespace foveated {

FoveatedAtlas& FoveatedAtlas::get() {
    static FoveatedAtlas instance;
    return instance;
}

bool FoveatedAtlas::initialize(ID3D12Device* dev, const AtlasConfig& cfg) {
    if (!dev) {
        spdlog::error("FoveatedAtlas::initialize: Device is null");
        return false;
    }
    
    m_cfg = cfg;
    
    // Calculate total dimensions (double-height for foveal+peripheral)
    const uint32_t totalWidth = m_cfg.fovealWidth * 2;
    const uint32_t totalHeight = m_cfg.fovealHeight + m_cfg.peripheralHeight;
    
    // Create render target resource
    D3D12_RESOURCE_DESC rtDesc{};
    rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rtDesc.Width = totalWidth;
    rtDesc.Height = totalHeight;
    rtDesc.DepthOrArraySize = 1;
    rtDesc.MipLevels = 1;
    rtDesc.Format = m_cfg.format;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.SampleDesc.Quality = 0;
    rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = m_cfg.format;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;
    
    HRESULT hr = dev->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &rtDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &clearValue,
        IID_PPV_ARGS(&m_atlas)
    );
    
    if (FAILED(hr)) {
        spdlog::error("FoveatedAtlas::initialize: Failed to create atlas texture");
        return false;
    }
    
    // Create depth buffer
    D3D12_RESOURCE_DESC depthDesc = rtDesc;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
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
        spdlog::error("FoveatedAtlas::initialize: Failed to create depth buffer");
        return false;
    }
    
    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    hr = dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr)) {
        spdlog::error("FoveatedAtlas::initialize: Failed to create RTV heap");
        return false;
    }
    
    // Create DSV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    hr = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
    if (FAILED(hr)) {
        spdlog::error("FoveatedAtlas::initialize: Failed to create DSV heap");
        return false;
    }
    
    // Create RTV
    m_rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    dev->CreateRenderTargetView(m_atlas.Get(), nullptr, m_rtv);
    
    // Create DSV
    m_dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    dev->CreateDepthStencilView(m_depth.Get(), nullptr, m_dsv);
    
    m_state = D3D12_RESOURCE_STATE_COMMON;
    
    spdlog::info("FoveatedAtlas initialized: {}x{} (format: {})", 
                 totalWidth, totalHeight, static_cast<int>(m_cfg.format));
    
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
    
    spdlog::info("FoveatedAtlas shutdown complete");
}

void FoveatedAtlas::transitionToRT(ID3D12GraphicsCommandList* cmd) {
    if (!cmd || !m_atlas) {
        return;
    }
    
    if (m_state != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_atlas.Get();
        barrier.Transition.StateBefore = m_state;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        
        cmd->ResourceBarrier(1, &barrier);
        m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
}

void FoveatedAtlas::transitionToSRV(ID3D12GraphicsCommandList* cmd) {
    if (!cmd || !m_atlas) {
        return;
    }
    
    if (m_state != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_atlas.Get();
        barrier.Transition.StateBefore = m_state;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        
        cmd->ResourceBarrier(1, &barrier);
        m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

} // namespace foveated
