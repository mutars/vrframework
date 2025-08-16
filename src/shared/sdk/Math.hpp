#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include <glm/gtx/rotate_vector.hpp>

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;
using Matrix3x3f = glm::mat3x3;
using Matrix3x4f = glm::mat3x4;
using Matrix4x3f = glm::mat4x3;
using Matrix4x4f = glm::mat4x4;

using Matrix3x4d = glm::dmat3x4;
using Matrix4x3d = glm::dmat4x3;
using Matrix4x4d = glm::dmat4x4;

using Vector4d = glm::dvec4;
using Vector3d = glm::dvec3;


static glm::vec3 euler_angles_from_steamvr(const glm::mat4x4& rot) {
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
    glm::extractEulerAngleZYX(rot, roll, yaw, pitch);

    return { pitch, -yaw, -roll };
}

static glm::vec3 euler_angles_from_steamvr(const glm::quat& q) {
    const auto m = glm::mat4x4{q};
    return euler_angles_from_steamvr(m);
}
