#include "VisibilityCache.hpp"

#include <algorithm>
#include <cstring>

namespace foveated {

VisibilityCache& VisibilityCache::get() {
    static VisibilityCache inst;
    return inst;
}

void VisibilityCache::initialize(uint32_t maxPrimitives) {
    std::unique_lock lock{m_mtx};
    const size_t bytes = (maxPrimitives + 7u) / 8u;
    for (auto& frame : m_frames) {
        for (auto& mask : frame.masks) {
            mask.resize(bytes);
        }
        frame.valid.fill(false);
    }
}

void VisibilityCache::beginFrame(int frameIdx) {
    std::unique_lock lock{m_mtx};
    const int frames = static_cast<int>(FRAMES);
    m_current = ((frameIdx % frames) + frames) % frames;
    m_frames[m_current].valid.fill(false);
}

void VisibilityCache::recordVisibility(uint32_t viewIdx, const uint8_t* bits, size_t count) {
    if (viewIdx >= VIEWS) {
        return;
    }
    if (bits == nullptr || count == 0) {
        return;
    }
    std::unique_lock lock{m_mtx};
    auto& frame = m_frames[m_current];
    auto& mask = frame.masks[viewIdx];
    if (mask.size() < count) {
        mask.resize(count);
    }
    std::memcpy(mask.data(), bits, count);
    frame.valid[viewIdx] = true;
}

const uint8_t* VisibilityCache::getVisibility(uint32_t viewIdx) const {
    if (viewIdx >= VIEWS) {
        return nullptr;
    }
    std::shared_lock lock{m_mtx};
    const auto& frame = m_frames[m_current];
    if (!frame.valid[viewIdx] || frame.masks[viewIdx].empty()) {
        return nullptr;
    }
    return frame.masks[viewIdx].data();
}

bool VisibilityCache::canShare(uint32_t src, uint32_t dst) const {
    return (src == 0 && dst == 2) || (src == 1 && dst == 3);
}

void VisibilityCache::copyVisibility(uint32_t src, uint32_t dst) {
    if (src >= VIEWS || dst >= VIEWS || !canShare(src, dst)) {
        return;
    }
    std::unique_lock lock{m_mtx};
    auto& frame = m_frames[m_current];
    if (!frame.valid[src]) {
        return;
    }
    frame.masks[dst] = frame.masks[src];
    frame.valid[dst] = true;
}

} // namespace foveated
