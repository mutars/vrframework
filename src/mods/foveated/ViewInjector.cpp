#include "ViewInjector.hpp"

#include <d3d12.h>

#include "D3D12Hook.hpp"
#include "FoveatedAtlas.hpp"

namespace foveated {

ViewInjector& ViewInjector::get() {
    static ViewInjector inst;
    return inst;
}

void ViewInjector::install(D3D12Hook* hook) {
    if (hook == nullptr) {
        return;
    }
    m_hook = hook;

    hook->on_set_viewports([this](D3D12Hook&, ID3D12GraphicsCommandList5* cmd, UINT num, const D3D12_VIEWPORT* vps) {
        onSetViewports(cmd, num, vps);
    });

    hook->on_set_render_targets([this](D3D12Hook&, ID3D12GraphicsCommandList5* cmd, UINT num,
                                       const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, BOOL single, D3D12_CPU_DESCRIPTOR_HANDLE* dsv) {
        onSetRenderTargets(cmd, num, rtvs, single, dsv);
    });
}

void ViewInjector::setAtlasRT(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
    m_atlasRT = rt;
    m_atlasRTV = rtv;
    if (m_atlasRT) {
        auto desc = m_atlasRT->GetDesc();
        m_atlasViewports = StereoEmulator::get().computeAtlasViewports(static_cast<uint32_t>(desc.Width), desc.Height);
    }
}

void ViewInjector::onSetViewports(ID3D12GraphicsCommandList5* cmd, UINT num, const D3D12_VIEWPORT* vps) {
    if (!cmd) {
        return;
    }

    const auto& stereo = StereoEmulator::get();
    if (m_atlasRT && stereo.isStereoActive()) {
        cmd->RSSetViewports(static_cast<UINT>(m_atlasViewports.size()), m_atlasViewports.data());
        return;
    }

    if (vps && num > 0) {
        cmd->RSSetViewports(num, vps);
    }
}

void ViewInjector::onSetRenderTargets(ID3D12GraphicsCommandList5* cmd, UINT num,
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, BOOL single, D3D12_CPU_DESCRIPTOR_HANDLE* dsv) {
    if (!cmd) {
        return;
    }

    const auto& stereo = StereoEmulator::get();
    if (m_atlasRT && stereo.isStereoActive()) {
        // Ensure atlas is bound
        cmd->OMSetRenderTargets(1, &m_atlasRTV, FALSE, dsv);
        if (!m_isRenderingFoveal) {
            injectFovealPair(cmd);
        } else {
            injectPeripheralPair(cmd);
        }
        m_isRenderingFoveal = !m_isRenderingFoveal;
        return;
    }

    if (rtvs && num > 0) {
        cmd->OMSetRenderTargets(num, rtvs, single, dsv);
    }
}

void ViewInjector::injectFovealPair(ID3D12GraphicsCommandList5* cmd) {
    if (!cmd) {
        return;
    }
    cmd->RSSetViewports(2, m_atlasViewports.data());
}

void ViewInjector::injectPeripheralPair(ID3D12GraphicsCommandList5* cmd) {
    if (!cmd) {
        return;
    }
    cmd->RSSetViewports(2, m_atlasViewports.data() + 2);
}

} // namespace foveated
