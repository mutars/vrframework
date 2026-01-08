#pragma once
#include "StereoEmulator.hpp"
#include <d3d12.h>

namespace foveated {

class D3D12Hook;

class ViewInjector {
public: 
    static ViewInjector& get();
    
    void install(class D3D12Hook* hook);
    void setAtlasRT(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv);

private:
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
