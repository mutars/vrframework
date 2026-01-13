#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdk {

struct FMinimalViewInfo {
    glm::vec3 Location{0.0f};
    glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};
    float FOV{90.0f};
};

struct APlayerCameraManager {};

} // namespace sdk
