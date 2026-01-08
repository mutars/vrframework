#include "StereoEmulator.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace foveated {

StereoEmulator& StereoEmulator::get() {
    static StereoEmulator inst;
    return inst;
}

void StereoEmulator::initialize(runtimes::VRRuntime* runtime) {
    m_runtime = runtime;

    m_views[0].type = ViewType::FOVEAL_LEFT_PRIMARY;
    m_views[0].stereoPassMask = 0x1;
    m_views[1].type = ViewType::FOVEAL_RIGHT_SECONDARY;
    m_views[1].stereoPassMask = 0x2;
    m_views[2].type = ViewType::PERIPHERAL_LEFT_PRIMARY;
    m_views[2].stereoPassMask = 0x1;
    m_views[3].type = ViewType::PERIPHERAL_RIGHT_SECONDARY;
    m_views[3].stereoPassMask = 0x2;
}

void StereoEmulator::configureFOV(float fovealDeg, float peripheralDeg) {
    m_fovealFov = fovealDeg;
    m_peripheralFov = peripheralDeg;
    m_fovealScale = glm::tan(glm::radians(fovealDeg * 0.5f));
    m_peripheralScale = glm::tan(glm::radians(peripheralDeg * 0.5f));
}

void StereoEmulator::beginFrame(int frameIndex) {
    if (m_runtime == nullptr) {
        m_stereoActive = false;
        return;
    }

    m_stereoActive = m_runtime->ready();
    if (!m_stereoActive) {
        return;
    }

    buildViewMatrices(frameIndex);
}

std::array<D3D12_VIEWPORT, 4> StereoEmulator::computeAtlasViewports(uint32_t width, uint32_t height) const {
    std::array<D3D12_VIEWPORT, 4> result{};

    const float halfWidth = static_cast<float>(width) * 0.5f;
    const float fovealHeight = static_cast<float>(height) * 0.5f;
    const float peripheralHeight = static_cast<float>(height) - fovealHeight;

    // Top row: foveal
    result[0] = {0.0f, 0.0f, halfWidth, fovealHeight, 0.0f, 1.0f};
    result[1] = {halfWidth, 0.0f, halfWidth, fovealHeight, 0.0f, 1.0f};

    // Bottom row: peripheral
    result[2] = {0.0f, fovealHeight, halfWidth, peripheralHeight, 0.0f, 1.0f};
    result[3] = {halfWidth, fovealHeight, halfWidth, peripheralHeight, 0.0f, 1.0f};

    return result;
}

void StereoEmulator::computeFrustumPlanes(EmulatedView& v) {
    // Simple placeholder extraction assuming column-major matrices
    const auto m = v.projection * v.view;
    // Left
    v.frustumPlanes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
    // Right
    v.frustumPlanes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
    // Bottom
    v.frustumPlanes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
    // Top
    v.frustumPlanes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
    // Near
    v.frustumPlanes[4] = glm::vec4(m[0][2], m[1][2], m[2][2], m[3][2]);
    // Far
    v.frustumPlanes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);
}

void StereoEmulator::buildViewMatrices(int) {
    // Basic left/right eye offsets derived from IPD; peripheral scaled by peripheral scale
    const float ipd = m_runtime ? m_runtime->get_ipd() : 0.064f;
    const float halfIpd = ipd * 0.5f;

    const glm::vec3 leftOffset{-halfIpd, 0.0f, 0.0f};
    const glm::vec3 rightOffset{halfIpd, 0.0f, 0.0f};

    m_views[0].view = glm::translate(glm::mat4{1.0f}, -leftOffset);
    m_views[1].view = glm::translate(glm::mat4{1.0f}, -rightOffset);
    m_views[2].view = glm::translate(glm::mat4{1.0f}, -leftOffset);
    m_views[3].view = glm::translate(glm::mat4{1.0f}, -rightOffset);

    const float nearZ = 0.05f;
    const float farZ = 1000.0f;

    const auto projFoveal = glm::perspective(glm::radians(m_fovealFov), 1.0f, nearZ, farZ);
    const auto projPeripheral = glm::perspective(glm::radians(m_peripheralFov), 1.0f, nearZ, farZ);

    m_views[0].projection = projFoveal;
    m_views[1].projection = projFoveal;
    m_views[2].projection = projPeripheral;
    m_views[3].projection = projPeripheral;

    m_views[0].fovScale = m_fovealScale;
    m_views[1].fovScale = m_fovealScale;
    m_views[2].fovScale = m_peripheralScale;
    m_views[3].fovScale = m_peripheralScale;

    for (auto& v : m_views) {
        computeFrustumPlanes(v);
    }
}

} // namespace foveated
