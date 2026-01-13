#pragma once

#include <array>
#include <glm/glm.hpp>
#include <d3d12.h>

#include "mods/vr/runtimes/VRRuntime.hpp"

namespace foveated {

enum class ViewType : uint8_t {
    FOVEAL_LEFT_PRIMARY = 0,
    FOVEAL_RIGHT_SECONDARY = 1,
    PERIPHERAL_LEFT_PRIMARY = 2,
    PERIPHERAL_RIGHT_SECONDARY = 3
};

struct EmulatedView {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec4 frustumPlanes[6]{};
    D3D12_VIEWPORT viewport{};
    float fovScale{1.0f};
    ViewType type{ViewType::FOVEAL_LEFT_PRIMARY};
    uint32_t stereoPassMask{0};
};

class StereoEmulator {
public:
    static StereoEmulator& get();

    void initialize(runtimes::VRRuntime* runtime);
    void beginFrame(int frameIndex);

    bool isStereoActive() const { return m_stereoActive; }
    const EmulatedView& getView(ViewType t) const { return m_views[static_cast<size_t>(t)]; }

    std::array<D3D12_VIEWPORT, 4> computeAtlasViewports(uint32_t width, uint32_t height) const;
    void configureFOV(float fovealDeg, float peripheralDeg);

private:
    void buildViewMatrices(int frame);
    void computeFrustumPlanes(EmulatedView& v);

    std::array<EmulatedView, 4> m_views{};
    runtimes::VRRuntime* m_runtime{nullptr};
    bool m_stereoActive{false};
    float m_fovealFov{40.f};
    float m_peripheralFov{110.f};
    float m_fovealScale{1.f};
    float m_peripheralScale{0.5f};
};

} // namespace foveated
