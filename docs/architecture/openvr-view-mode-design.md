# OpenVR View Mode Architecture Design

## Problem Statement

OpenXR supports a flat screen display mode via `XrCompositionLayerQuad`, controlled by
`ModSettings::showFlatScreenDisplay()`. This renders the game as a 2D billboard in VR space
instead of stereo projection. OpenVR lacks this capability — `ModSettings.cpp:12` explicitly
returns `false` for OpenVR with a `//TODO fix for openvr` comment.

The challenge: OpenXR switches between stereo and flat modes within the same `xrEndFrame()` call
(swapping composition layer types). OpenVR has no equivalent — its `VRCompositor()->Submit()` only
supports stereo projection. Flat display in OpenVR requires routing textures through the entirely
separate `IVROverlay` API.

## Plans Evaluated

### Plan A: Full Abstraction — Unified ViewSubmitter

Introduces a `ViewSubmitter` interface that encapsulates the entire frame submission pipeline.
Both runtimes implement it. D3D components delegate to it instead of calling runtime APIs directly.

**New files**: `ViewSubmitter.hpp`, `OpenXRViewSubmitter.hpp/.cpp`, `OpenVRViewSubmitter.hpp/.cpp`
**Files modified**: ~12 total

**Pros:**
- D3D components become runtime-agnostic for submission
- Eliminates `is_openxr()`/`is_openvr()` branching in submission paths
- Clean extension point for future view modes

**Cons:**
- Restructures working stereo submission paths that have subtle AER timing dependencies
- The abstraction is leaky: OpenVR flat mode uses `IVROverlay` (not compositor), so
  `submit_eye_texture()` becomes a no-op for the right eye and `end_frame()` calls a
  different API entirely
- High regression risk touching both D3D11 and D3D12 submission logic

**Risk: HIGH** — Rejected.

### Plan B: Minimal Change — OverlayComponent Extension (Recommended)

Extends `OverlayComponent` with a game-view overlay handle. D3D components get a small conditional
to redirect OpenVR texture submission to the overlay when flat mode is active. Uses the existing
`ModSettings::showFlatScreenDisplay()` as the sole control flag — no new enums, no runtime state
machine, no capability queries.

**New files**: none
**Files modified**: ~6 total

**Pros:**
- Small diff, leverages existing overlay infrastructure
- Doesn't touch stereo submission paths
- Low risk, fast to implement
- Single control flag (`showFlatScreenDisplay()`) governs both runtimes

**Cons:**
- Duplicates flat-screen positioning logic between OpenXR.cpp and OverlayComponent
- OpenVR overlay quality is lower than compositor (no ATW/reprojection) — platform limitation

**Risk: LOW**

### Plan C: Hybrid — ViewMode on VRRuntime

Adds a `ViewMode` enum and virtual methods (`active_view_mode`, `set_view_mode`,
`supports_view_mode`) to `VRRuntime`. Each runtime implements mode switching natively.

**New files**: none
**Files modified**: ~8 total

**Analysis:** Overengineered for the current need. We support two runtimes and they're aligned
on capabilities. Adding a ViewMode state machine, capability queries, and notification patterns
when `showFlatScreenDisplay()` already exists as the control flag adds unnecessary abstraction.
The extra indirection doesn't pay for itself when the D3D components already branch on runtime
type anyway.

**Risk: MEDIUM-LOW** — Rejected (unnecessary complexity).

## Recommended Plan: B (Minimal OverlayComponent Extension)

The existing `ModSettings::showFlatScreenDisplay()` already controls flat mode for OpenXR.
The only work is:
1. Remove the OpenVR guard so the flag applies to both runtimes
2. Add a game-view overlay to `OverlayComponent` for OpenVR's `IVROverlay` path
3. Add guards in D3D components to route OpenVR textures to the overlay instead of compositor

No new enums. No new virtual methods on VRRuntime. No mode state machine.

## Current Architecture (Relevant Paths)

```
Frame Submission Flow (per eye):
  VR::on_present()
  ├── synchronize_frame()    [VRRuntime virtual]
  ├── update_poses()         [VRRuntime virtual]
  └── D3DxxComponent::on_frame()
      ├── Copy backbuffer → eye texture
      ├── if OpenXR: m_openxr.copy(eye_index, texture)
      ├── if OpenVR: VRCompositor()->Submit(eye, texture, bounds)
      └── if right_eye or async_aer:
          ├── OpenXR: end_frame(quad_layers, frame, has_depth)
          │   ├── if !showFlatScreenDisplay(): XrCompositionLayerProjection
          │   └── if  showFlatScreenDisplay(): XrCompositionLayerQuad  ← exists
          └── OpenVR: (no end_frame / no flat mode)                   ← gap
```

## Proposed Architecture

### Step 1: Remove OpenVR Guard in ModSettings

```
File: src/ModSettings.cpp

Before:
  bool showFlatScreenDisplay() {
      static auto vr = VR::get();
      //TODO fix for openvr
      if (vr->is_hmd_active() && vr->get_runtime()->is_openvr()) {
          return false;
      }
      return g_internalSettings.forceFlatScreen || g_internalSettings.showQuadDisplay;
  }

After:
  bool showFlatScreenDisplay() {
      return g_internalSettings.forceFlatScreen || g_internalSettings.showQuadDisplay;
  }
```

This makes `showFlatScreenDisplay()` runtime-agnostic. OpenXR's `end_frame()` at
`OpenXR.cpp:1420` already checks this flag and needs no changes.

### Step 2: Add Game View Overlay to OverlayComponent

```
File: src/mods/vr/OverlayComponent.hpp

Add:
  vr::VROverlayHandle_t m_game_view_overlay{vr::k_ulOverlayHandleInvalid};
  bool m_game_view_active{false};

  void activate_game_view_overlay();
  void deactivate_game_view_overlay();
  void submit_game_view_texture_d3d11(ID3D11Texture2D* tex, VRRuntime* runtime);
  void submit_game_view_texture_d3d12(ID3D12Resource* tex, ID3D12CommandQueue* queue,
                                      VRRuntime* runtime);
```

```
File: src/mods/vr/OverlayComponent.cpp

on_initialize_openvr() — add after existing overlay creation:
  Create "VRFramework_GameView" overlay via VROverlay()->CreateOverlay()
  Set width to 2.0m (matches OpenXR quad layer size)
  Set input method to VROverlayInputMethod_None
  Keep hidden initially (HideOverlay)

activate_game_view_overlay():
  VROverlay()->ShowOverlay(m_game_view_overlay)
  m_game_view_active = true

deactivate_game_view_overlay():
  VROverlay()->HideOverlay(m_game_view_overlay)
  m_game_view_active = false

submit_game_view_texture_d3d11(tex, runtime):
  if (!m_game_view_active) activate_game_view_overlay()

  // Set texture
  vr::Texture_t overlay_tex{(void*)tex, vr::TextureType_DirectX, vr::ColorSpace_Auto}
  VROverlay()->SetOverlayTexture(m_game_view_overlay, &overlay_tex)

  // Position: reuse same math as OpenXR (OpenXR.cpp:1472-1478)
  flat_screen = glm::mat4(1.0f)
  flat_screen[3][2] = -runtime->m_flat_screen_distance
  flat_screen = runtime->m_center_stage * flat_screen
  Convert to HmdMatrix34_t
  VROverlay()->SetOverlayTransformAbsolute(m_game_view_overlay,
      vr::TrackingUniverseStanding, &transform)

  // Maintain aspect ratio (matches OpenXR quad: 2.0m wide)
  VROverlay()->SetOverlayWidthInMeters(m_game_view_overlay, 2.0f)

submit_game_view_texture_d3d12(tex, queue, runtime):
  Same as d3d11 but wraps texture in vr::D3D12TextureData_t{tex, queue, 0}
  Uses vr::TextureType_DirectX12
```

### Step 3: D3D Component Guards

The key change: when `showFlatScreenDisplay()` is true and runtime is OpenVR, route
the left eye texture to the overlay and skip `VRCompositor()->Submit()`.

Only the left eye texture is submitted (same as OpenXR which uses a single quad with
`XR_EYE_VISIBILITY_BOTH`). The right eye compositor submission is also skipped.

```
File: src/mods/vr/D3D11Component.cpp

In left-eye OpenVR block (~line 308):
  if (runtime->is_openvr()) {
      if (ModSettings::showFlatScreenDisplay()) {
          auto& overlay = vr->get_overlay_component();
          overlay.submit_game_view_texture_d3d11(m_left_eye_tex.Get(), runtime);
      } else {
          // existing VRCompositor()->Submit() code unchanged
      }
  }

In right-eye OpenVR block (~line 380):
  if (runtime->is_openvr()) {
      if (ModSettings::showFlatScreenDisplay()) {
          // skip compositor submit — overlay already showing left eye
      } else {
          // existing submission code unchanged
      }
  }

File: src/mods/vr/D3D12Component.cpp

Same pattern in OpenVR blocks (~lines 97, 163):
  Left eye: route to overlay.submit_game_view_texture_d3d12()
  Right eye: skip compositor submit
```

### Step 4: Handle Mode Transitions

When `showFlatScreenDisplay()` transitions from true→false, the game view overlay
must be hidden so the compositor stereo view is visible again.

```
File: src/mods/vr/D3D11Component.cpp / D3D12Component.cpp

In the OpenVR submission blocks, after the showFlatScreenDisplay() check:
  if (!ModSettings::showFlatScreenDisplay() && overlay.m_game_view_active) {
      overlay.deactivate_game_view_overlay();
  }
```

Alternatively, this can be checked once per frame in `OverlayComponent::on_pre_imgui_frame()`
which already runs every frame.

### Step 5: Enable UI Sliders for OpenVR

```
File: src/mods/VR.cpp

The flat screen distance slider (~line 1175) and any related controls should be
visible regardless of runtime type. If they are currently gated behind OpenXR
checks, remove those guards.
```

## Important Considerations

### OpenVR Overlay Limitations

OpenVR overlays differ from compositor submissions:
- No asynchronous timewarp (ATW) reprojection — head movement causes visible latency
- Not tied to headset refresh rate for updates
- Lower visual fidelity than native compositor path

These are platform limitations. Users should be aware flat mode on OpenVR will have
slightly worse visual quality than on OpenXR.

### WaitGetPoses in Flat Mode

Even when not submitting via `VRCompositor()->Submit()`, `WaitGetPoses()` must still be
called each frame. Failing to do so causes `VRCompositorError_DoNotHaveFocus`. The existing
`synchronize_frame()` in `OpenVR.cpp:6` handles this and continues running regardless of
whether we submit to the compositor or overlay.

### Overlay Conflict Prevention

The existing `OverlayComponent` manages a framework UI overlay and a slate overlay.
The new game-view overlay must:
- Use a distinct overlay key (`"VRFramework_GameView"`)
- Not interfere with framework UI overlay visibility
- Be hidden when flat mode is deactivated

### AER Interaction

When flat mode is active, AER (Alternate Eye Rendering) frame parity still controls which
eye texture gets rendered. But since we only submit the left eye to the overlay quad (visible
to both eyes), the right eye frame can either:
- Also submit the same left eye texture (simplest — slight latency on alternating frames)
- Be skipped entirely for overlay submission (only update on left eye frames)

The second approach is cleaner: only submit to the overlay on left-eye frames, do nothing
on right-eye frames. This matches how OpenXR handles it — the quad layer uses a single
swapchain image visible to both eyes.

## Implementation Order

1. Add game-view overlay to `OverlayComponent` (create during init, show/hide/submit methods)
2. Add guards in `D3D11Component.cpp` OpenVR submission blocks
3. Add guards in `D3D12Component.cpp` OpenVR submission blocks
4. Remove OpenVR guard from `ModSettings.cpp`
5. Enable UI sliders for OpenVR in `VR.cpp`
6. Test stereo mode still works on both runtimes (regression)
7. Test flat mode on OpenVR

## Key File References

| File | Lines | What to Change |
|------|-------|----------------|
| `src/ModSettings.cpp` | 12-14 | Remove OpenVR guard |
| `src/mods/vr/OverlayComponent.hpp` | 75-78 | Add game view overlay handle + methods |
| `src/mods/vr/OverlayComponent.cpp` | 15-71 | Init game view overlay, add submit methods |
| `src/mods/vr/D3D11Component.cpp` | 308-337 | Add flat mode guard (left eye) |
| `src/mods/vr/D3D11Component.cpp` | 380-414 | Add flat mode guard (right eye) |
| `src/mods/vr/D3D12Component.cpp` | 97-143 | Add flat mode guard (left eye) |
| `src/mods/vr/D3D12Component.cpp` | 163-211 | Add flat mode guard (right eye) |
| `src/mods/VR.cpp` | 1175-1192 | Enable UI sliders for OpenVR |
| `src/mods/vr/runtimes/OpenXR.cpp` | 1420 | No changes — already uses showFlatScreenDisplay() |
| `src/mods/vr/runtimes/OpenVR.cpp` | — | No changes needed |
