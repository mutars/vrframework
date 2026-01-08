# 4-View Foveated ISR Pipeline Implementation

This implementation provides a complete 4-View Foveated Instanced Stereo Rendering (ISR) pipeline for VR applications, along with an example game integration template.

## Overview

The foveated rendering system enables high-quality VR rendering by using four views:
- **Foveal views** (left/right): High resolution, narrow field of view for central vision
- **Peripheral views** (left/right): Lower resolution, wide field of view for peripheral vision

This approach optimizes rendering performance by allocating more resources to the user's central vision while maintaining a wide field of view.

## Architecture

### Core Components (`src/mods/foveated/`)

#### 1. StereoEmulator (203 lines)
Manages 4 emulated views with projection matrices and frustum culling data.

**Key Features:**
- Configurable FOV for foveal (default 40°) and peripheral (default 110°) views
- Per-view projection and view matrices
- Frustum plane computation for culling
- Integration with VRRuntime for IPD and pose data

**Usage:**
```cpp
auto& emulator = foveated::StereoEmulator::get();
emulator.initialize(vrRuntime);
emulator.configureFOV(40.0f, 110.0f);
emulator.beginFrame(frameIndex);
```

#### 2. FoveatedAtlas (218 lines)
Creates and manages the double-height render target atlas for 4-view output.

**Atlas Layout:**
```
+-------------------+-------------------+
| Foveal Left       | Foveal Right      |  <- High resolution
| (1440x1600)       | (1440x1600)       |
+-------------------+-------------------+
| Peripheral Left   | Peripheral Right  |  <- Lower resolution
| (720x800)         | (720x800)         |
+-------------------+-------------------+
Total: 2880x2400
```

**Key Features:**
- Configurable resolution per view type
- Automatic descriptor heap creation (RTV/DSV)
- Resource state tracking and transitions
- Depth buffer support

**Usage:**
```cpp
auto& atlas = foveated::FoveatedAtlas::get();
foveated::AtlasConfig config{};
config.fovealWidth = 1440;
config.fovealHeight = 1600;
atlas.initialize(device, config);
```

#### 3. ViewInjector (140 lines)
Hooks D3D12 rendering pipeline to redirect output to the atlas.

**Key Features:**
- Viewport injection for 4-view rendering
- Render target redirection to atlas
- Integration with D3D12Hook callback system
- Separate rendering passes for foveal and peripheral pairs

**Usage:**
```cpp
auto& injector = foveated::ViewInjector::get();
injector.install(d3d12Hook);
injector.setAtlasRT(atlas.getTexture(), atlas.getRTV());
```

#### 4. VisibilityCache (158 lines)
Optimizes rendering by sharing culling results between view pairs.

**Key Features:**
- Per-view visibility bitmask storage
- 3-frame history for temporal coherence
- Thread-safe operations with shared_mutex
- Visibility sharing: Foveal → Peripheral for same eye

**Usage:**
```cpp
auto& cache = foveated::VisibilityCache::get();
cache.initialize(maxPrimitives);
cache.beginFrame(frameIndex);
cache.recordVisibility(viewIdx, visibilityBits, count);
if (cache.canShare(0, 2)) {
    cache.copyVisibility(0, 2); // Left foveal → Left peripheral
}
```

## Game Integration Template (`src/games/ExampleUE/`)

This template demonstrates how to integrate the foveated rendering system into an Unreal Engine-based game.

### Structure

```
src/games/ExampleUE/
├── ExampleUEEntry.h/cpp           # Mod entry point
├── ExampleUERendererModule.h/cpp  # Frame/render lifecycle hooks
├── ExampleUECameraModule.h/cpp    # Camera transform hooks
├── memory/
│   └── offsets.h                  # Pattern-based function location
└── sdk/
    └── ExampleUESDK.h            # Reversed engine structures
```

### Integration Points

#### 1. ExampleUEEntry (44 lines)
Main mod entry point that inherits from `Mod`.

**Features:**
- Module initialization and hook installation
- UI configuration (HUD scale, decoupled pitch)
- Follows vrframework mod pattern

#### 2. ExampleUERendererModule (73 lines)
Hooks game rendering lifecycle events.

**Key Hook Points:**
- `BeginFrame`: Frame counter synchronization
- `BeginRender`: VR state update, HMD tracking

**Integration:**
```cpp
vr->m_engine_frame_count++;
vr->on_begin_rendering(vr->m_render_frame_count);
vr->update_hmd_state(vr->m_render_frame_count);
```

#### 3. ExampleUECameraModule (87 lines)
Hooks camera calculations for VR transform injection.

**Key Hook Points:**
- `CalcView`: Apply HMD pose to camera
- `GetProjection`: Submit VR projection matrices

**Integration:**
```cpp
auto eye = vr->get_current_eye_transform();
auto hmd = vr->get_transform(0);
GlobalPool::submit_projection(*outProj, vr->m_render_frame_count);
```

### Memory Offsets (31 lines)
Pattern-based function location using signature scanning.

**Example:**
```cpp
inline uintptr_t beginFrameAddr() {
    return FuncRelocation("BeginFrame", 
        "48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 E8", 0x0);
}
```

### SDK Structures (41 lines)
Reversed engine structures for camera and view data.

**Example:**
```cpp
struct FMinimalViewInfo {
    FVector Location;
    FRotator Rotation;
    float FOV;
    float AspectRatio;
};
```

## Integration Checklist

To integrate this system into a new game:

1. **Create game folder**: `src/games/{GameName}/`
2. **Define offsets**: Find function addresses via signature scanning
3. **Implement modules**:
   - Entry point (`{GameName}Entry`)
   - Renderer hooks (`{GameName}RendererModule`)
   - Camera hooks (`{GameName}CameraModule`)
4. **Hook targets** (minimum required):
   - Frame begin (increment frame counters)
   - Render begin (call `VR::on_begin_rendering`)
   - Projection matrix calc (inject VR projection)
   - View matrix calc (inject HMD transform)

## Technical Details

### Thread Safety
- VisibilityCache uses `shared_mutex` for concurrent read access
- StereoEmulator and FoveatedAtlas are designed for single-threaded access during frame rendering

### Performance Considerations
- Foveal views: Full resolution (e.g., 1440x1600)
- Peripheral views: Half resolution (e.g., 720x800)
- Visibility cache enables skipping redundant culling operations
- Mega-frustum optimization shares culling between view pairs

### Compatibility
- Requires D3D12 for rendering
- Integrates with existing vrframework VRRuntime abstraction
- Compatible with OpenXR and OpenVR backends
- Designed for Unreal Engine games but adaptable to other engines

## Statistics

- **Total Lines of Code**: 1,043
- **Core Components**: 767 lines
- **Game Integration Template**: 276 lines
- **Files Created**: 16

## References

This implementation follows the integration pattern from the `mutars/acsvr` repository and implements the design specification provided in the problem statement.

## Future Enhancements

Potential improvements for production use:

1. **Dynamic Resolution**: Adjust foveal/peripheral resolution based on performance
2. **Gaze Tracking**: Real-time foveation based on eye tracking
3. **Advanced Culling**: Hierarchical culling with mega-frustum
4. **Multi-GPU**: Split foveal/peripheral rendering across GPUs
5. **Shader Integration**: Coordinate with ISR shader passes
