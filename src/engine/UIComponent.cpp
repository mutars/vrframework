#include "UIComponent.h"
#include <cmath>
#include <engine/models/ModSettings.h>
#include <glm/vec4.hpp>
#include <safetyhook/easy.hpp>
#include <spdlog/spdlog.h>

using GetCamera_t   = uintptr_t(*)(int);
using GetOrphoFov_t = float(*)(uintptr_t camera);

namespace sdk {
    auto mod = (uintptr_t)utility::get_executable();

    struct UICanvasInfo
    {
        // Unknown data
        char padding_0[0x84];

        // The horizontal anchor position in the reference UI coordinate space.
        // Accessed via: *(unsigned int *)(v3 + 132)
        float anchorX; // Offset: 0x84 (132)

        // The vertical anchor position in the reference UI coordinate space.
        // Accessed via: *(unsigned int *)(v3 + 136)
        float anchorY; // Offset: 0x88 (136)

        // ... other canvas-related data may follow
    };

    // Verify offsets
    static_assert(offsetof(UICanvasInfo, anchorX) == 0x84, "Incorrect anchorX offset");
    static_assert(offsetof(UICanvasInfo, anchorY) == 0x88, "Incorrect anchorY offset");

    struct UIParentController
    {
        // Unknown data
        char padding_0[0x28]; // 40 bytes

        // Pointer to the object containing anchor data.
        // Accessed via: *(_QWORD *)(*(_QWORD *)(a1 + 48) + 40i64)
        // The inner dereference gets this object, the outer gets pCanvasInfo.
        UICanvasInfo* pCanvasInfo; // Offset: 0x28 (40)

        // ...
    };

    // Verify offset
    static_assert(offsetof(UIParentController, pCanvasInfo) == 0x28, "Incorrect pCanvasInfo offset");

    struct GameObject
    {
        // Unknown data
        char padding_0[0x30];

        // Pointer to a parent or controller object.
        // Accessed via: *(_QWORD *)(a1 + 48)
        UIParentController* pParentController; // Offset: 0x30 (48)

        // Padding to reach the next known member
        char padding_1[0x90 - 0x30 - sizeof(UIParentController*)];

        glm::vec4 positionalData; // Offset: 0x90 (144)
        // locks and other
    };

    // Verify all offsets
    static_assert(offsetof(GameObject, pParentController) == 0x30, "Incorrect pParentController offset");
    static_assert(offsetof(GameObject, positionalData) == 0x90, "Incorrect initialPositionData offset");

    uintptr_t* GetCamera(int index) {
        static auto fn = (GetCamera_t)(mod + 0x540ab0);
        return (uintptr_t*)(fn(index) + 0x300);
    }
    float GetOrthoFov(uintptr_t camera) {
        static auto fn = (GetOrphoFov_t)(mod + 0x692730);
        return fn(camera);
    }
}



void UIComponent::InstallHooks() {
    spdlog::info("Installing UIComponent hooks");

//    uintptr_t setGOScreenEdgeFn = (uintptr_t)utility::get_executable() + 0x58e1c0;
//    m_onSetGOScreenEdge = safetyhook::create_inline((void*)setGOScreenEdgeFn, (void*)&UIComponent::onSetGOScreenEdge);
}


/*
 function SetGOScreenEdge(sdk::GameObject object):
   // 1. SETUP
   // Get the object's "authored" position from its parent canvas.
   // These anchors are the ideal coordinates on a 16:9 reference screen, stored in world units.
   canvas_info = object.pParentController.pCanvasInfo
   anchor_x = canvas_info.anchorX
   anchor_y = canvas_info.anchorY

// Calculate how the current aspect ratio compares to the 16:9 standard.
//  - value < 1.0 means the screen is TALLER than 16:9 (e.g., 4:3).
//  - value = 1.0 means the screen is exactly 16:9.
//  - value > 1.0 means the screen is WIDER than 16:9 (e.g., 21:9).
normalized_ratio = get_current_aspect_ratio() * (9.0 / 16.0)


                  // 2. MAIN LOGIC
                  // The final position starts as the object's current position, and will be modified.
                  final_position = object.positionalData.row[3]

     if normalized_ratio <= 1.0:
   // --- CASE: Screen is TALLER than 16:9 ---
   // We need to adjust the VERTICAL position to stick to the top/bottom edges.

   // Calculate how much extra vertical space exists compared to the 16:9 reference.
   vertical_offset = calculate_vertical_offset(normalized_ratio)

   // Define a "safe zone" in the center. Only adjust if the object is outside it.
   vertical_threshold = calculate_vertical_edge_threshold()

         if anchor_y > vertical_threshold: // Is it near the top edge?
                            final_position.y = anchor_y + vertical_offset
         else if anchor_y < -vertical_threshold: // Is it near the bottom edge?
                             final_position.y = anchor_y - vertical_offset

     else:
   // --- CASE: Screen is WIDER than 16:9 ---
   // We need to adjust the HORIZONTAL position to stick to the left/right edges.

   // Respect ultrawide settings that may cap the maximum aspect ratio.
   effective_ratio = apply_ultrawide_cap(normalized_ratio)

   // Calculate how much extra horizontal space exists.
   horizontal_offset = calculate_horizontal_offset(effective_ratio)

   // Define a "safe zone" in the center.
   horizontal_threshold = calculate_horizontal_edge_threshold()

         if anchor_x > horizontal_threshold: // Is it near the right edge?
                              final_position.x = anchor_x + horizontal_offset
         else if anchor_x < -horizontal_threshold: // Is it near the left edge?
                               final_position.x = anchor_x - horizontal_offset


                    // 3. FINALIZE
                    // Write the newly calculated position back into the object's transform matrix.
                    object.positionalData.row[3] = final_position

     // Notify the engine that the transform has been updated.
     unlock_transform(object.positionalData)
 */
uintptr_t UIComponent::onSetGOScreenEdge(sdk::GameObject* a1)
{
    static auto instance = UIComponent::Get();
    auto response = instance->m_onSetGOScreenEdge.call<uintptr_t>(a1);
    
    // Apply circular boundary constraint for VR if enabled
    if (a1 && a1->pParentController && a1->pParentController->pCanvasInfo) {
        instance->ApplyCircularBoundary(a1);
    }
    
    return response;
}

void UIComponent::ApplyCircularBoundary(sdk::GameObject* object) {
    // Get current position
    glm::vec4& position = object->positionalData;
    float x = position.x;
    float y = position.y;
    
    // Calculate distance from center (0, 0)
    float distance = std::sqrt(x * x + y * y);
    
    // Use dynamic radius calculation if available, otherwise use configured radius
    float activeRadius = CalculateOptimalRadius();
    if (activeRadius <= 0.0f) {
        return;
    }
    
    // Check if the element is outside the circular boundary
    if (distance > activeRadius) {
        // Calculate the angle from center to the element
        float angle = std::atan2(y, x);
        
        // Clamp the element to the circular boundary
        position.x = activeRadius * std::cos(angle);
        position.y = activeRadius * std::sin(angle);
        
        spdlog::debug("UI element clamped to VR circular boundary: ({:.2f}, {:.2f}) -> ({:.2f}, {:.2f}), radius: {:.2f}", 
                     x, y, position.x, position.y, activeRadius);
    }
}

float UIComponent::CalculateOptimalRadius() {
    // Try to get camera information to calculate optimal radius
    try {
        auto camera = sdk::GetCamera(2);
        if (camera) {
            float orthoFov = sdk::GetOrthoFov(*camera);
            if (orthoFov > 0) {
                // Calculate radius based on orthographic FOV
                // This assumes the UI is positioned at a certain distance from the camera
                // Adjust the multiplier based on your specific VR setup
                float radius = orthoFov * 0.5f * ModSettings::g_UISettings.hudScale; // 75% of the FOV for comfortable viewing
                spdlog::debug("Calculated optimal VR radius: {:.2f} based on ortho FOV: {:.2f}", radius, orthoFov);
                return radius;
            }
        }
    }
    catch (...) {
        spdlog::warn("Failed to get camera information for VR radius calculation");
    }
    
    // Fallback to default radius
    return 0.0;
}
