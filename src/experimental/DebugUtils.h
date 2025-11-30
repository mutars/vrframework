#pragma once
#include <glm/fwd.hpp>

namespace DebugUtils
{
    struct DebugConfig
    {
        bool      debugShaders{ false };
        bool      cameraShake{ false };
        bool      cameraSpin{ false };
        bool      cameraMotion{ true };
    };
    extern DebugConfig config;
    glm::mat4 generateOffsetMatrix();
};
