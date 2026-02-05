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

**Risk: HIGH**

### Plan B: Minimal Change — OverlayComponent Extension

Extends `OverlayComponent` with a game-view overlay handle. D3D components get a small conditional
to redirect OpenVR texture submission to the overlay when flat mode is active.

**New files**: none
**Files modified**: ~6 total

**Pros:**
- Small diff, leverages existing overlay infrastructure
- Doesn't touch stereo submission paths
- Low risk, fast to implement

**Cons:**
- No abstraction — future view modes (cylinder, passthrough) require more ad-hoc branches
- Duplicates flat-screen positioning logic between OpenXR.cpp and OverlayComponent
- D3D component branching complexity grows with each new mode

**Risk: LOW**

### Plan C: Hybrid — ViewMode on VRRuntime (Recommended)

Adds a `ViewMode` enum and virtual methods to `VRRuntime`. Each runtime implements mode switching
natively. OpenXR uses its existing composition layer system. OpenVR uses `IVROverlay` managed
through `OverlayComponent`.

**New files**: none
**Files modified**: ~8 total

**Pros:**
- View mode is a first-class runtime concept at the right abstraction level
- Doesn't restructure working stereo paths (only adds guard conditions)
- Naturally extensible for cylinder, passthrough, multi-view
- Respects the genuine asymmetry between OpenVR and OpenXR APIs

**Cons:**
- D3D components still need runtime-type checks for OpenVR flat mode
- OpenVR overlay quality is lower than compositor (no ATW/reprojection)

**Risk: MEDIUM-LOW**

## Recommended Plan: C (Hybrid) with OverlayComponent Delegation

Plan C with one refinement from Plan B: the `OpenVR` runtime declares the mode, but
`OverlayComponent` manages the actual OpenVR overlay lifecycle (create/show/hide/texture).
This keeps overlay management consolidated in the class that already handles it.

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
          │   ├── if STEREO: XrCompositionLayerProjection
          │   └── if FLAT:   XrCompositionLayerQuad    ← exists
          └── OpenVR: (no end_frame equivalent)        ← gap
```

## Proposed Architecture

### Step 1: ViewMode Enum on VRRuntime

```
File: src/mods/vr/runtimes/VRRuntime.hpp

Add:
  enum class ViewMode : uint8_t {
    STEREO_PROJECTION,
    FLAT_QUAD,
    FLAT_CYLINDER,
  };

  virtual ViewMode active_view_mode() const;
  virtual bool supports_view_mode(ViewMode mode) const;
  virtual void set_view_mode(ViewMode mode);
```

### Step 2: OpenXR Implementation

```
File: src/mods/vr/runtimes/OpenXR.cpp

Change end_frame() line 1420:
  Before: if (!ModSettings::showFlatScreenDisplay())
  After:  if (active_view_mode() == ViewMode::STEREO_PROJECTION)

Add FLAT_CYLINDER case using XrCompositionLayerCylinderKHR
(extension already checked via is_cylinder_layer_allowed())
```

### Step 3: OpenVR Implementation

```
File: src/mods/vr/runtimes/OpenVR.hpp

Add:
  ViewMode m_view_mode{ViewMode::STEREO_PROJECTION};
  void set_view_mode(ViewMode mode) override;
  ViewMode active_view_mode() const override;
  bool supports_view_mode(ViewMode mode) const override;

File: src/mods/vr/runtimes/OpenVR.cpp

  supports_view_mode() returns true for STEREO, FLAT_QUAD, FLAT_CYLINDER
  set_view_mode() notifies OverlayComponent to show/hide game view overlay
```

### Step 4: OverlayComponent Game View Overlay

```
File: src/mods/vr/OverlayComponent.hpp

Add:
  vr::VROverlayHandle_t m_game_view_overlay{k_ulOverlayHandleInvalid};
  bool m_game_view_active{false};

  void activate_game_view_overlay(ViewMode mode);
  void deactivate_game_view_overlay();
  void submit_game_view_texture_d3d11(ID3D11Texture2D* tex);
  void submit_game_view_texture_d3d12(ID3D12Resource* tex, ID3D12CommandQueue* queue);
  void update_game_view_transform(float distance, const Matrix4x4f& center_stage);

File: src/mods/vr/OverlayComponent.cpp

  activate_game_view_overlay():
    Create overlay if needed (VROverlay()->CreateOverlay)
    ShowOverlay, SetOverlayWidthInMeters
    If FLAT_CYLINDER: SetOverlayCurvature()

  submit_game_view_texture_d3d11():
    SetOverlayTexture with DirectX texture
    Update transform (distance + center_stage)

  deactivate_game_view_overlay():
    HideOverlay
```

### Step 5: D3D Component Guards

```
File: src/mods/vr/D3D11Component.cpp

In OpenVR submission blocks (~lines 308, 380):
  if (runtime->active_view_mode() != ViewMode::STEREO_PROJECTION) {
    overlay.submit_game_view_texture_d3d11(eye_tex);
    // skip VRCompositor()->Submit()
  } else {
    // existing submission code unchanged
  }

File: src/mods/vr/D3D12Component.cpp

Same pattern in OpenVR blocks (~lines 97, 163)
```

### Step 6: ModSettings and UI

```
File: src/ModSettings.cpp

Remove OpenVR guard (lines 12-14).
showFlatScreenDisplay() drives set_view_mode() via VR.cpp.

File: src/mods/VR.cpp

Enable flat screen distance slider for OpenVR (currently OpenXR-only)
Enable FOV scale sliders for OpenVR
Wire ModSettings changes to runtime->set_view_mode()
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
`synchronize_frame()` handles this and should continue running regardless of view mode.

### Overlay Conflict Prevention

The existing `OverlayComponent` manages framework UI overlay and slate overlay. The new
game-view overlay must:
- Use a distinct overlay key (e.g., "VRFramework_GameView")
- Not interfere with framework UI overlay visibility
- Be hidden when returning to stereo mode

### Cylinder Mode Details

OpenVR: `IVROverlay::SetOverlayCurvature(handle, curvature)` where curvature is [0..1],
with 1 being a fully closed cylinder. Curvature = width / (2 * PI * radius).

OpenXR: `XrCompositionLayerCylinderKHR` with explicit radius, centralAngle, aspectRatio.
Requires `XR_KHR_composition_layer_cylinder` extension (already checked by
`is_cylinder_layer_allowed()`).

## Implementation Order

1. Add `ViewMode` enum and virtual methods to `VRRuntime.hpp`
2. Implement `OpenVR::set_view_mode()` / `supports_view_mode()` / `active_view_mode()`
3. Add game-view overlay to `OverlayComponent` (create, show, hide, submit texture)
4. Add guards in `D3D11Component.cpp` OpenVR submission blocks
5. Add guards in `D3D12Component.cpp` OpenVR submission blocks
6. Refactor `OpenXR::end_frame()` to use `active_view_mode()`
7. Remove OpenVR guard from `ModSettings.cpp`
8. Enable UI sliders for OpenVR in `VR.cpp`
9. Test stereo mode still works on both runtimes (regression)
10. Test flat mode on OpenVR
11. Add `FLAT_CYLINDER` support (incremental follow-up)

## Key File References

| File | Lines | Relevance |
|------|-------|-----------|
| `src/ModSettings.cpp` | 8-16 | OpenVR guard to remove |
| `src/mods/vr/runtimes/VRRuntime.hpp` | 14-167 | Base class — add ViewMode |
| `src/mods/vr/runtimes/OpenXR.cpp` | 1392-1513 | end_frame() — refactor mode check |
| `src/mods/vr/runtimes/OpenVR.hpp` | 6-54 | Add view mode state |
| `src/mods/vr/runtimes/OpenVR.cpp` | 6-194 | Add view mode methods |
| `src/mods/vr/OverlayComponent.hpp` | 16-165 | Add game view overlay |
| `src/mods/vr/OverlayComponent.cpp` | 15-71 | Init game view overlay |
| `src/mods/vr/D3D11Component.cpp` | 308-337, 380-414 | Add flat mode guards |
| `src/mods/vr/D3D12Component.cpp` | 97-143, 163-211 | Add flat mode guards |
| `src/mods/VR.cpp` | 1175-1192 | Enable UI for OpenVR |
| `extern/openvr/headers/openvr.h` | 3849, 3944-3949 | IVROverlay API reference |
