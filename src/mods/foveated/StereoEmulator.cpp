#include "StereoEmulator.hpp"
#include <cmath>
#include <algorithm>

namespace foveated {

void StereoEmulator::initialize(VRRuntime* runtime) {
    m_runtime = runtime;
    m_stereoActive = (runtime != nullptr && runtime->ready());
    
    // Initialize views with default values
    for (size_t i = 0; i < 4; ++i) {
        m_views[i].type = static_cast<ViewType>(i);
        m_views[i].stereoPassMask = (i % 2 == 0) ? 0x1 : 0x2; // Primary or Secondary
        m_views[i].fovScale = (i < 2) ? m_fovealScale : m_peripheralScale;
    }
}

void StereoEmulator::beginFrame(int frameIndex) {
    if (!m_stereoActive || !m_runtime) {
        return;
    }
    
    buildViewMatrices(frameIndex);
}

void StereoEmulator::buildViewMatrices(int frame) {
    if (!m_runtime || !m_runtime->ready()) {
        return;
    }
    
    // Get eye transforms from runtime
    const auto& eyes = m_runtime->eyes;
    
    // Build foveal views (higher resolution, narrower FOV)
    m_views[0].view = eyes[0]; // Left eye
    m_views[1].view = eyes[1]; // Right eye
    
    // Peripheral views share the same eye positions but with wider FOV
    m_views[2].view = eyes[0]; // Left peripheral
    m_views[3].view = eyes[1]; // Right peripheral
    
    // Compute frustum planes for culling optimization
    for (auto& view : m_views) {
        computeFrustumPlanes(view);
    }
}

void StereoEmulator::computeFrustumPlanes(EmulatedView& v) {
    // Extract frustum planes from projection * view matrix for culling
    glm::mat4 vp = v.projection * v.view;
    
    // Left plane
    v.frustumPlanes[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]
    );
    
    // Right plane
    v.frustumPlanes[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]
    );
    
    // Bottom plane
    v.frustumPlanes[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
    );
    
    // Top plane
    v.frustumPlanes[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    );
    
    // Near plane
    v.frustumPlanes[4] = glm::vec4(
        vp[0][2],
        vp[1][2],
        vp[2][2],
        vp[3][2]
    );
    
    // Far plane
    v.frustumPlanes[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(glm::vec3(v.frustumPlanes[i]));
        if (len > 0.0f) {
            v.frustumPlanes[i] /= len;
        }
    }
}

std::array<D3D12_VIEWPORT, 4> StereoEmulator::computeAtlasViewports(uint32_t w, uint32_t h) const {
    std::array<D3D12_VIEWPORT, 4> viewports{};
    
    // Atlas layout: [FovealL|FovealR] top, [PeriphL|PeriphR] bottom
    float halfWidth = static_cast<float>(w) / 2.0f;
    float fovealHeight = static_cast<float>(h) * 0.6f; // 60% for foveal
    float peripheralHeight = static_cast<float>(h) * 0.4f; // 40% for peripheral
    
    // Foveal Left (top-left)
    viewports[0] = {0.0f, 0.0f, halfWidth, fovealHeight, 0.0f, 1.0f};
    
    // Foveal Right (top-right)
    viewports[1] = {halfWidth, 0.0f, halfWidth, fovealHeight, 0.0f, 1.0f};
    
    // Peripheral Left (bottom-left)
    viewports[2] = {0.0f, fovealHeight, halfWidth, peripheralHeight, 0.0f, 1.0f};
    
    // Peripheral Right (bottom-right)
    viewports[3] = {halfWidth, fovealHeight, halfWidth, peripheralHeight, 0.0f, 1.0f};
    
    return viewports;
}

void StereoEmulator::configureFOV(float fovealDeg, float peripheralDeg) {
    m_fovealFov = fovealDeg;
    m_peripheralFov = peripheralDeg;
    
    // Update scales based on FOV ratio
    // Guard against near-zero values that could cause extreme scale ratios
    constexpr float MIN_FOV_DEG = 1.0f;
    if (peripheralDeg >= MIN_FOV_DEG) {
        m_fovealScale = 1.0f;
        m_peripheralScale = std::clamp(fovealDeg / peripheralDeg, 0.1f, 1.0f);
    }
    
    // Update view fov scales
    for (size_t i = 0; i < 4; ++i) {
        m_views[i].fovScale = (i < 2) ? m_fovealScale : m_peripheralScale;
    }
}

} // namespace foveated
