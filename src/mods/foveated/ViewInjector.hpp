#pragma once

#include "StereoEmulator.hpp"
#include <d3d12.h>
#include <array>

class D3D12Hook;

namespace foveated {

class ViewInjector {
public:
    static ViewInjector& get() {
        static ViewInjector instance;
        return instance;
    }

    void install(D3D12Hook* hook);
    void setAtlasRT(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    
    bool isInstalled() const { return m_hook != nullptr; }
    bool isRenderingFoveal() const { return m_isRenderingFoveal; }
    
    void setRenderingFoveal(bool foveal) { m_isRenderingFoveal = foveal; }
    
    const std::array<D3D12_VIEWPORT, 4>& getAtlasViewports() const { 
        return m_atlasViewports; 
    }
    
    void updateAtlasViewports(uint32_t width, uint32_t height);

private:
    ViewInjector() = default;
    ~ViewInjector() = default;
    
    ViewInjector(const ViewInjector&) = delete;
    ViewInjector& operator=(const ViewInjector&) = delete;

    void onSetViewports(ID3D12GraphicsCommandList5* cmd, UINT num, const D3D12_VIEWPORT* vps);
    void onSetRenderTargets(ID3D12GraphicsCommandList5* cmd, UINT num,
        const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, BOOL single, D3D12_CPU_DESCRIPTOR_HANDLE* dsv);
    void injectFovealPair(ID3D12GraphicsCommandList5* cmd);
    void injectPeripheralPair(ID3D12GraphicsCommandList5* cmd);

    D3D12Hook* m_hook{nullptr};
    ID3D12Resource* m_atlasRT{nullptr};
    D3D12_CPU_DESCRIPTOR_HANDLE m_atlasRTV{};
    std::array<D3D12_VIEWPORT, 4> m_atlasViewports{};
    bool m_isRenderingFoveal{false};
};

} // namespace foveated
