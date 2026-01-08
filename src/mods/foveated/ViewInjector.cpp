#include "ViewInjector.hpp"
#include "D3D12Hook.hpp"

namespace foveated {

void ViewInjector::install(D3D12Hook* hook) {
    m_hook = hook;
    
    if (!hook) {
        return;
    }
    
    // The actual D3D12 hook callbacks are registered through the framework's
    // existing hook system. This class provides the logic for viewport/RT injection.
}

void ViewInjector::setAtlasRT(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    m_atlasRT = rt;
    m_atlasRTV = rtv;
}

void ViewInjector::updateAtlasViewports(uint32_t width, uint32_t height) {
    m_atlasViewports = StereoEmulator::get().computeAtlasViewports(width, height);
}

void ViewInjector::onSetViewports(ID3D12GraphicsCommandList5* cmd, UINT num, const D3D12_VIEWPORT* vps) {
    if (!StereoEmulator::get().isStereoActive() || !m_atlasRT) {
        return;
    }
    
    // Redirect viewports to atlas regions based on current rendering pass
    if (m_isRenderingFoveal) {
        injectFovealPair(cmd);
    } else {
        injectPeripheralPair(cmd);
    }
}

void ViewInjector::onSetRenderTargets(ID3D12GraphicsCommandList5* cmd, UINT num,
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, BOOL single, D3D12_CPU_DESCRIPTOR_HANDLE* dsv) {
    
    if (!StereoEmulator::get().isStereoActive() || !m_atlasRT) {
        return;
    }
    
    // Redirect render targets to atlas RT
    // The actual RT redirection is handled by the caller
}

void ViewInjector::injectFovealPair(ID3D12GraphicsCommandList5* cmd) {
    if (!cmd) {
        return;
    }
    
    // Set viewports for foveal pair (views 0 and 1)
    // ISR Draw Pair A: views 0+1 (Foveal Left + Foveal Right)
    std::array<D3D12_VIEWPORT, 2> fovealViewports = {
        m_atlasViewports[0],
        m_atlasViewports[1]
    };
    
    cmd->RSSetViewports(2, fovealViewports.data());
}

void ViewInjector::injectPeripheralPair(ID3D12GraphicsCommandList5* cmd) {
    if (!cmd) {
        return;
    }
    
    // Set viewports for peripheral pair (views 2 and 3)
    // ISR Draw Pair B: views 2+3 (Peripheral Left + Peripheral Right)
    std::array<D3D12_VIEWPORT, 2> peripheralViewports = {
        m_atlasViewports[2],
        m_atlasViewports[3]
    };
    
    cmd->RSSetViewports(2, peripheralViewports.data());
}

} // namespace foveated
