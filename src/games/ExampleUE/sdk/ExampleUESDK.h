#pragma once
#include <glm/glm.hpp>

// Example SDK structures for Unreal Engine
// These would typically be generated from UE4SS dumps or manual reverse engineering

namespace sdk {

// Simplified FVector for demonstration
struct FVector {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

// Simplified FRotator for demonstration  
struct FRotator {
    float pitch{0.0f};
    float yaw{0.0f};
    float roll{0.0f};
};

// Simplified FMinimalViewInfo structure
struct FMinimalViewInfo {
    FVector Location;
    FRotator Rotation;
    float FOV{90.0f};
    float AspectRatio{1.777778f};
    float OrthoWidth{512.0f};
    float OrthoNearClipPlane{0.0f};
    float OrthoFarClipPlane{10000.0f};
};

// Simplified APlayerCameraManager class
struct APlayerCameraManager {
    // This would contain the actual class structure
    // For demonstration purposes, we're keeping it minimal
    char _pad[0x100];
};

} // namespace sdk
