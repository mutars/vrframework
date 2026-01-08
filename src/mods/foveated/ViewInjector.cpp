#include "ViewInjector.hpp"
#include "D3D12Hook.hpp"
#include <spdlog/spdlog.h>

namespace foveated {

ViewInjector& ViewInjector::get() {
    static ViewInjector instance;
    return instance;
}

void ViewInjector::install(D3D12Hook* hook) {
    if (!hook) {
        spdlog::error("ViewInjector::install: D3D12Hook is null");
        return;
    }
    
    m_hook = hook;
    
    // Register callbacks with D3D12Hook
    m_hook->on_set_viewports([this](D3D12Hook& hook, ID3D12GraphicsCommandList5* cmd, 
                                     UINT num, const D3D12_VIEWPORT* vps) {
        this->onSetViewports(cmd, num, vps);
    });
    
    m_hook->on_set_render_targets([this](D3D12Hook& hook, ID3D12GraphicsCommandList5* cmd,
                                          UINT num, const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
                                          BOOL single, D3D12_CPU_DESCRIPTOR_HANDLE* dsv) {
        this->onSetRenderTargets(cmd, num, rtvs, single, dsv);
    });
    
    spdlog::info("ViewInjector installed hooks");
}

void ViewInjector::setAtlasRT(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    m_atlasRT = rt;
    m_atlasRTV = rtv;
    
    if (m_atlasRT) {
        auto desc = m_atlasRT->GetDesc();
        m_atlasViewports = StereoEmulator::get().computeAtlasViewports(
            static_cast<uint32_t>(desc.Width), 
            static_cast<uint32_t>(desc.Height)
        );
        spdlog::info("ViewInjector atlas RT set: {}x{}", desc.Width, desc.Height);
    }
}

void ViewInjector::onSetViewports(ID3D12GraphicsCommandList5* cmd, UINT num, 
                                   const D3D12_VIEWPORT* vps) {
    // Check if we should inject foveated viewports
    auto& emulator = StereoEmulator::get();
    if (!emulator.isStereoActive() || !m_atlasRT) {
        return;
    }
    
    // Intercept viewport changes and redirect to atlas viewports if needed
    // This is where we would implement the actual viewport injection logic
    // For now, this is a placeholder for the hook mechanism
}

void ViewInjector::onSetRenderTargets(ID3D12GraphicsCommandList5* cmd, UINT num,
                                       const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, BOOL single,
                                       D3D12_CPU_DESCRIPTOR_HANDLE* dsv) {
    // Check if we should redirect to atlas render target
    auto& emulator = StereoEmulator::get();
    if (!emulator.isStereoActive() || !m_atlasRT) {
        return;
    }
    
    // Intercept render target changes and redirect to atlas if needed
    // This is where we would implement the actual RT injection logic
    // For now, this is a placeholder for the hook mechanism
}

void ViewInjector::injectFovealPair(ID3D12GraphicsCommandList5* cmd) {
    if (!cmd || !m_atlasRT) {
        return;
    }
    
    // Set viewports for foveal pair (views 0 and 1)
    D3D12_VIEWPORT fovealVPs[2] = {
        m_atlasViewports[0], // Foveal left
        m_atlasViewports[1]  // Foveal right
    };
    
    cmd->RSSetViewports(2, fovealVPs);
    cmd->OMSetRenderTargets(1, &m_atlasRTV, FALSE, nullptr);
    
    m_isRenderingFoveal = true;
}

void ViewInjector::injectPeripheralPair(ID3D12GraphicsCommandList5* cmd) {
    if (!cmd || !m_atlasRT) {
        return;
    }
    
    // Set viewports for peripheral pair (views 2 and 3)
    D3D12_VIEWPORT peripheralVPs[2] = {
        m_atlasViewports[2], // Peripheral left
        m_atlasViewports[3]  // Peripheral right
    };
    
    cmd->RSSetViewports(2, peripheralVPs);
    cmd->OMSetRenderTargets(1, &m_atlasRTV, FALSE, nullptr);
    
    m_isRenderingFoveal = false;
}

} // namespace foveated
