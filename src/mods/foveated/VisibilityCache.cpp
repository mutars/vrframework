#include "VisibilityCache.hpp"
#include <algorithm>
#include <cstring>

namespace foveated {

void VisibilityCache::initialize(uint32_t maxPrimitives) {
    std::unique_lock lock(m_mtx);
    
    m_maxPrimitives = maxPrimitives;
    
    // Calculate bytes needed for bit mask (1 bit per primitive)
    size_t bytesNeeded = (maxPrimitives + 7) / 8;
    
    for (auto& frame : m_frames) {
        for (auto& mask : frame.masks) {
            mask.resize(bytesNeeded, 0);
        }
        frame.valid.fill(false);
    }
    
    m_current = 0;
}

void VisibilityCache::beginFrame(int frameIdx) {
    std::unique_lock lock(m_mtx);
    
    m_current = frameIdx % FRAMES;
    
    // Reset validity for the new frame
    m_frames[m_current].valid.fill(false);
    
    // Clear visibility masks
    for (auto& mask : m_frames[m_current].masks) {
        std::fill(mask.begin(), mask.end(), static_cast<uint8_t>(0));
    }
}

void VisibilityCache::recordVisibility(uint32_t viewIdx, const uint8_t* bits, size_t count) {
    if (viewIdx >= VIEWS || !bits || count == 0) {
        return;
    }
    
    std::unique_lock lock(m_mtx);
    
    auto& frame = m_frames[m_current];
    auto& mask = frame.masks[viewIdx];
    
    size_t bytesToCopy = std::min(count, mask.size());
    std::memcpy(mask.data(), bits, bytesToCopy);
    
    frame.valid[viewIdx] = true;
}

const uint8_t* VisibilityCache::getVisibility(uint32_t viewIdx) const {
    if (viewIdx >= VIEWS) {
        return nullptr;
    }
    
    std::shared_lock lock(m_mtx);
    
    const auto& frame = m_frames[m_current];
    if (!frame.valid[viewIdx]) {
        return nullptr;
    }
    
    return frame.masks[viewIdx].data();
}

bool VisibilityCache::canShare(uint32_t src, uint32_t dst) const {
    // Mega-frustum optimization: foveal views can share culling with peripheral views
    // View 0 (Foveal Left) can share with View 2 (Peripheral Left)
    // View 1 (Foveal Right) can share with View 3 (Peripheral Right)
    
    if (src == 0 && dst == 2) return true;
    if (src == 1 && dst == 3) return true;
    
    return false;
}

void VisibilityCache::copyVisibility(uint32_t src, uint32_t dst) {
    if (!canShare(src, dst)) {
        return;
    }
    
    std::unique_lock lock(m_mtx);
    
    auto& frame = m_frames[m_current];
    
    if (!frame.valid[src]) {
        return;
    }
    
    // Copy visibility from source to destination
    frame.masks[dst] = frame.masks[src];
    frame.valid[dst] = true;
}

bool VisibilityCache::isValid(uint32_t viewIdx) const {
    if (viewIdx >= VIEWS) {
        return false;
    }
    
    std::shared_lock lock(m_mtx);
    return m_frames[m_current].valid[viewIdx];
}

size_t VisibilityCache::getVisibilityCount(uint32_t viewIdx) const {
    if (viewIdx >= VIEWS) {
        return 0;
    }
    
    std::shared_lock lock(m_mtx);
    return m_frames[m_current].masks[viewIdx].size();
}

} // namespace foveated
