#pragma once
#include <glm/ext/matrix_float4x4.hpp>

namespace GlobalPool
{
    static const int CONSTANTS_HISTORY_SIZE = 10;

    struct Constants
    {
        glm::mat4     projection{ 1.0 };
        glm::mat4     view{ 1.0f };
        glm::mat4     eyeView{ 1.0f };
        struct {
            glm::mat4 prevClipToClip{1.0f};
            glm::mat4 cameraViewToClip{1.0f};
        } sl_constants{};
    };

    extern Constants g_constants[CONSTANTS_HISTORY_SIZE];

    inline void submit_hmd_view(const glm::mat4& view, int frame)
    {
        g_constants[frame % CONSTANTS_HISTORY_SIZE].view = view;
    }

    inline void submit_eye_view(const glm::mat4& view, int frame)
    {
        g_constants[frame % CONSTANTS_HISTORY_SIZE].eyeView = view;
    }

    inline auto& get_projection(int frame)
    {
        return g_constants[frame % CONSTANTS_HISTORY_SIZE].projection;
    }

    inline auto& get_hmd_view(int frame)
    {
        return g_constants[frame % CONSTANTS_HISTORY_SIZE].view;
    }

    inline auto& get_eye_view(int frame)
    {
        return g_constants[frame % CONSTANTS_HISTORY_SIZE].eyeView;
    }

//    inline void submitSlConstants(const sl::Constants& constants, uint32_t frame)
//    {
//        g_constants[frame % CONSTANTS_HISTORY_SIZE].sl_constants = constants;
//    }

    inline auto& getSlConstants(uint32_t frame)
    {
        return g_constants[frame % CONSTANTS_HISTORY_SIZE].sl_constants;
    }

}; // namespace GlobalPool
