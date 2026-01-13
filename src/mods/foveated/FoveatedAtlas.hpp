#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace foveated {

/**
 * Atlas configuration using percentages of VR render size
 * Both foveal and peripheral views use the same texture dimensions,
 * controlled by horizontal and vertical scale percentages.
 */
struct AtlasConfig {
    // Scale percentages relative to VR runtime's reported width/height
    // These control the atlas size as a percentage of vr->get_hmd_width()/get_hmd_height()
    float horizontalScale{1.0f};  // 1.0 = 100% of VR width per eye
    float verticalScale{1.0f};    // 1.0 = 100% of VR height per eye
    
    // Base dimensions from VR runtime (set via initializeFromVR)
    uint32_t baseWidth{0};   // From vr->get_hmd_width()
    uint32_t baseHeight{0};  // From vr->get_hmd_height()
    
    DXGI_FORMAT format{DXGI_FORMAT_R10G10B10A2_UNORM};
    
    // Computed eye dimensions (same for foveal and peripheral)
    uint32_t getEyeWidth() const { 
        return static_cast<uint32_t>(baseWidth * horizontalScale); 
    }
    uint32_t getEyeHeight() const { 
        return static_cast<uint32_t>(baseHeight * verticalScale); 
    }
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
    
    // Atlas layout: 2 columns (L/R) x 2 rows (Foveal/Peripheral)
    // All 4 views have the same dimensions
    uint32_t getTotalWidth() const { return m_cfg.getEyeWidth() * 2; }
    uint32_t getTotalHeight() const { return m_cfg.getEyeHeight() * 2; }
    
    // All views share the same dimensions
    uint32_t getEyeWidth() const { return m_cfg.getEyeWidth(); }
    uint32_t getEyeHeight() const { return m_cfg.getEyeHeight(); }
    
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
