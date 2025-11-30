#pragma once
#include <glm/ext/matrix_float4x4.hpp>

namespace GlobalPool
{
    static const int CONSTANTS_HISTORY_SIZE = 10;

    struct Constants
    {
        glm::mat4     projection{ 1.0 };
        glm::mat4     finalView{ 1.0f };
    };

    extern Constants g_constants[CONSTANTS_HISTORY_SIZE];

    glm::mat4 get_correction_matrix(int frame, int past_frame);

    inline void submit_projection(glm::mat4& projection, int frame)
    {
        g_constants[frame % CONSTANTS_HISTORY_SIZE].projection = projection;
    }

    inline void submit_final_view(glm::mat4& finalView, int frame)
    {
        g_constants[frame % CONSTANTS_HISTORY_SIZE].finalView = finalView;
    }

    inline auto& get_projection(int frame)
    {
        return g_constants[frame % CONSTANTS_HISTORY_SIZE].projection;
    }

    inline const auto& get_final_view(int frame)
    {
        return g_constants[frame % CONSTANTS_HISTORY_SIZE].finalView;
    }

}; // namespace GlobalPool
