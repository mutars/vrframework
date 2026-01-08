#pragma once

#include <array>
#include <vector>
#include <shared_mutex>
#include <cstdint>

namespace foveated {

class VisibilityCache {
public:
    static VisibilityCache& get() {
        static VisibilityCache instance;
        return instance;
    }

    void initialize(uint32_t maxPrimitives);
    void beginFrame(int frameIdx);
    
    void recordVisibility(uint32_t viewIdx, const uint8_t* bits, size_t count);
    const uint8_t* getVisibility(uint32_t viewIdx) const;
    
    // Check if visibility can be shared between views (0->2, 1->3)
    // Foveal left can share with peripheral left, foveal right with peripheral right
    bool canShare(uint32_t src, uint32_t dst) const;
    void copyVisibility(uint32_t src, uint32_t dst);
    
    bool isValid(uint32_t viewIdx) const;
    size_t getVisibilityCount(uint32_t viewIdx) const;

private:
    VisibilityCache() = default;
    ~VisibilityCache() = default;
    
    VisibilityCache(const VisibilityCache&) = delete;
    VisibilityCache& operator=(const VisibilityCache&) = delete;

    static constexpr size_t FRAMES = 3;
    static constexpr size_t VIEWS = 4;
    
    struct Frame {
        std::array<std::vector<uint8_t>, VIEWS> masks{};
        std::array<bool, VIEWS> valid{};
    };
    
    std::array<Frame, FRAMES> m_frames{};
    int m_current{0};
    uint32_t m_maxPrimitives{0};
    mutable std::shared_mutex m_mtx;
};

} // namespace foveated
