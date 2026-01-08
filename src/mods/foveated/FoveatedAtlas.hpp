#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace foveated {

struct AtlasConfig {
    uint32_t fovealWidth{1440};
    uint32_t fovealHeight{1600};
    uint32_t peripheralWidth{720};
    uint32_t peripheralHeight{800};
    DXGI_FORMAT format{DXGI_FORMAT_R10G10B10A2_UNORM};
};

class FoveatedAtlas {
public:
    static FoveatedAtlas& get() {
        static FoveatedAtlas instance;
        return instance;
    }

    bool initialize(ID3D12Device* dev, const AtlasConfig& cfg);
    void shutdown();
    
    ID3D12Resource* getTexture() const { return m_atlas.Get(); }
    ID3D12Resource* getDepthTexture() const { return m_depth.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const { return m_rtv; }
    D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const { return m_dsv; }
    
    uint32_t getTotalWidth() const { return m_cfg.fovealWidth * 2; }
    uint32_t getTotalHeight() const { return m_cfg.fovealHeight + m_cfg.peripheralHeight; }
    
    uint32_t getFovealWidth() const { return m_cfg.fovealWidth; }
    uint32_t getFovealHeight() const { return m_cfg.fovealHeight; }
    uint32_t getPeripheralWidth() const { return m_cfg.peripheralWidth; }
    uint32_t getPeripheralHeight() const { return m_cfg.peripheralHeight; }
    
    const AtlasConfig& getConfig() const { return m_cfg; }
    
    void transitionToRT(ID3D12GraphicsCommandList* cmd);
    void transitionToSRV(ID3D12GraphicsCommandList* cmd);
    
    bool isInitialized() const { return m_atlas.Get() != nullptr; }

private:
    FoveatedAtlas() = default;
    ~FoveatedAtlas() = default;
    
    FoveatedAtlas(const FoveatedAtlas&) = delete;
    FoveatedAtlas& operator=(const FoveatedAtlas&) = delete;

    AtlasConfig m_cfg{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_atlas;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depth;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv{};
    D3D12_RESOURCE_STATES m_state{D3D12_RESOURCE_STATE_COMMON};
};

} // namespace foveated
