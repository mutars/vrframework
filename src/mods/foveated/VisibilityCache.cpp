#include "VisibilityCache.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace foveated {

VisibilityCache& VisibilityCache::get() {
    static VisibilityCache instance;
    return instance;
}

void VisibilityCache::initialize(uint32_t maxPrimitives) {
    std::unique_lock lock(m_mtx);
    
    // Calculate number of bytes needed for bitmask
    const size_t numBytes = (maxPrimitives + 7) / 8;
    
    // Initialize all frames and views
    for (auto& frame : m_frames) {
        for (size_t v = 0; v < VIEWS; ++v) {
            frame.masks[v].resize(numBytes, 0);
            frame.valid[v] = false;
        }
    }
    
    m_current = 0;
    spdlog::info("VisibilityCache initialized for {} primitives ({} bytes per view)", 
                 maxPrimitives, numBytes);
}

void VisibilityCache::beginFrame(int frameIdx) {
    std::unique_lock lock(m_mtx);
    
    m_current = frameIdx % FRAMES;
    
    // Invalidate current frame's views
    auto& currentFrame = m_frames[m_current];
    for (size_t v = 0; v < VIEWS; ++v) {
        currentFrame.valid[v] = false;
        // Clear the mask
        std::fill(currentFrame.masks[v].begin(), currentFrame.masks[v].end(), 0);
    }
}

void VisibilityCache::recordVisibility(uint32_t viewIdx, const uint8_t* bits, size_t count) {
    if (viewIdx >= VIEWS || !bits) {
        return;
    }
    
    std::unique_lock lock(m_mtx);
    
    auto& currentFrame = m_frames[m_current];
    auto& mask = currentFrame.masks[viewIdx];
    
    // Copy visibility data
    const size_t copySize = std::min(count, mask.size());
    std::memcpy(mask.data(), bits, copySize);
    
    currentFrame.valid[viewIdx] = true;
}

const uint8_t* VisibilityCache::getVisibility(uint32_t viewIdx) const {
    if (viewIdx >= VIEWS) {
        return nullptr;
    }
    
    std::shared_lock lock(m_mtx);
    
    const auto& currentFrame = m_frames[m_current];
    if (!currentFrame.valid[viewIdx]) {
        return nullptr;
    }
    
    return currentFrame.masks[viewIdx].data();
}

bool VisibilityCache::canShare(uint32_t src, uint32_t dst) const {
    if (src >= VIEWS || dst >= VIEWS) {
        return false;
    }
    
    // Can share visibility between foveal and peripheral views of the same eye
    // 0 (Foveal Left) -> 2 (Peripheral Left)
    // 1 (Foveal Right) -> 3 (Peripheral Right)
    if ((src == 0 && dst == 2) || (src == 1 && dst == 3)) {
        std::shared_lock lock(m_mtx);
        const auto& currentFrame = m_frames[m_current];
        return currentFrame.valid[src];
    }
    
    return false;
}

void VisibilityCache::copyVisibility(uint32_t src, uint32_t dst) {
    if (src >= VIEWS || dst >= VIEWS) {
        return;
    }
    
    // Can share visibility between foveal and peripheral views of the same eye
    // 0 (Foveal Left) -> 2 (Peripheral Left)
    // 1 (Foveal Right) -> 3 (Peripheral Right)
    if ((src != 0 || dst != 2) && (src != 1 || dst != 3)) {
        return;
    }
    
    std::unique_lock lock(m_mtx);
    
    auto& currentFrame = m_frames[m_current];
    
    // Check validity while holding the lock
    if (!currentFrame.valid[src]) {
        return;
    }
    
    const auto& srcMask = currentFrame.masks[src];
    auto& dstMask = currentFrame.masks[dst];
    
    // Copy visibility mask
    dstMask = srcMask;
    currentFrame.valid[dst] = true;
}

} // namespace foveated
