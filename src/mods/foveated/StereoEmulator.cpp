#include "StereoEmulator.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace foveated {

StereoEmulator& StereoEmulator::get() {
    static StereoEmulator instance;
    return instance;
}

void StereoEmulator::initialize(VRRuntime* runtime) {
    if (!runtime) {
        spdlog::error("StereoEmulator::initialize: VRRuntime is null");
        return;
    }
    
    m_runtime = runtime;
    m_stereoActive = true;
    
    // Initialize view types and stereo pass masks
    m_views[0].type = ViewType::FOVEAL_LEFT_PRIMARY;
    m_views[0].stereoPassMask = 0x1;
    m_views[0].fovScale = m_fovealScale;
    
    m_views[1].type = ViewType::FOVEAL_RIGHT_SECONDARY;
    m_views[1].stereoPassMask = 0x2;
    m_views[1].fovScale = m_fovealScale;
    
    m_views[2].type = ViewType::PERIPHERAL_LEFT_PRIMARY;
    m_views[2].stereoPassMask = 0x1;
    m_views[2].fovScale = m_peripheralScale;
    
    m_views[3].type = ViewType::PERIPHERAL_RIGHT_SECONDARY;
    m_views[3].stereoPassMask = 0x2;
    m_views[3].fovScale = m_peripheralScale;
    
    spdlog::info("StereoEmulator initialized with foveal FOV: {}, peripheral FOV: {}", 
                 m_fovealFov, m_peripheralFov);
}

void StereoEmulator::beginFrame(int frameIndex) {
    if (!m_runtime || !m_stereoActive) {
        return;
    }
    
    buildViewMatrices(frameIndex);
    
    // Compute frustum planes for all views
    for (auto& view : m_views) {
        computeFrustumPlanes(view);
    }
}

void StereoEmulator::buildViewMatrices(int frame) {
    // Get HMD pose from VR runtime
    // Note: This is a simplified implementation. In a real scenario,
    // you would get the actual pose from the VR runtime
    
    const float ipd = m_runtime->get_ipd();
    const float halfIPD = ipd * 0.5f;
    
    // Build projection matrices
    const float fovealRadians = glm::radians(m_fovealFov);
    const float peripheralRadians = glm::radians(m_peripheralFov);
    const float aspect = 1.0f; // Will be adjusted based on actual render target
    const float nearZ = 0.1f;
    const float farZ = 1000.0f;
    
    // Foveal projections (narrow FOV, high resolution)
    m_views[0].projection = glm::perspective(fovealRadians, aspect, nearZ, farZ);
    m_views[1].projection = glm::perspective(fovealRadians, aspect, nearZ, farZ);
    
    // Peripheral projections (wide FOV, lower resolution)
    m_views[2].projection = glm::perspective(peripheralRadians, aspect, nearZ, farZ);
    m_views[3].projection = glm::perspective(peripheralRadians, aspect, nearZ, farZ);
    
    // Build view matrices with eye offsets
    // Left eye views
    glm::mat4 leftEyeOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-halfIPD, 0.0f, 0.0f));
    m_views[0].view = leftEyeOffset;
    m_views[2].view = leftEyeOffset;
    
    // Right eye views
    glm::mat4 rightEyeOffset = glm::translate(glm::mat4(1.0f), glm::vec3(halfIPD, 0.0f, 0.0f));
    m_views[1].view = rightEyeOffset;
    m_views[3].view = rightEyeOffset;
}

void StereoEmulator::computeFrustumPlanes(EmulatedView& v) {
    // Compute frustum planes from view-projection matrix
    glm::mat4 vp = v.projection * v.view;
    
    // Extract frustum planes (left, right, top, bottom, near, far)
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
    
    // Top plane
    v.frustumPlanes[2] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    );
    
    // Bottom plane
    v.frustumPlanes[3] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
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
        float length = glm::length(glm::vec3(v.frustumPlanes[i]));
        if (length > 0.0f) {
            v.frustumPlanes[i] /= length;
        }
    }
}

std::array<D3D12_VIEWPORT, 4> StereoEmulator::computeAtlasViewports(uint32_t w, uint32_t h) const {
    std::array<D3D12_VIEWPORT, 4> viewports{};
    
    // Atlas layout: [FovealL|FovealR] top, [PeriphL|PeriphR] bottom
    const float halfWidth = static_cast<float>(w) * 0.5f;
    const float halfHeight = static_cast<float>(h) * 0.5f;
    
    // Foveal left (top-left)
    viewports[0].TopLeftX = 0.0f;
    viewports[0].TopLeftY = 0.0f;
    viewports[0].Width = halfWidth;
    viewports[0].Height = halfHeight;
    viewports[0].MinDepth = 0.0f;
    viewports[0].MaxDepth = 1.0f;
    
    // Foveal right (top-right)
    viewports[1].TopLeftX = halfWidth;
    viewports[1].TopLeftY = 0.0f;
    viewports[1].Width = halfWidth;
    viewports[1].Height = halfHeight;
    viewports[1].MinDepth = 0.0f;
    viewports[1].MaxDepth = 1.0f;
    
    // Peripheral left (bottom-left)
    viewports[2].TopLeftX = 0.0f;
    viewports[2].TopLeftY = halfHeight;
    viewports[2].Width = halfWidth;
    viewports[2].Height = halfHeight;
    viewports[2].MinDepth = 0.0f;
    viewports[2].MaxDepth = 1.0f;
    
    // Peripheral right (bottom-right)
    viewports[3].TopLeftX = halfWidth;
    viewports[3].TopLeftY = halfHeight;
    viewports[3].Width = halfWidth;
    viewports[3].Height = halfHeight;
    viewports[3].MinDepth = 0.0f;
    viewports[3].MaxDepth = 1.0f;
    
    return viewports;
}

void StereoEmulator::configureFOV(float fovealDeg, float peripheralDeg) {
    m_fovealFov = fovealDeg;
    m_peripheralFov = peripheralDeg;
    spdlog::info("StereoEmulator FOV configured - Foveal: {}, Peripheral: {}", 
                 m_fovealFov, m_peripheralFov);
}

} // namespace foveated
