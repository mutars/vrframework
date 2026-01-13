# 4-View Foveated ISR Pipeline - Design Specification

## Requirements

### Prerequisites
- UE SDK headers generated (via UE4SS dump or manual RE)
- `vrframework` as submodule/dependency
- Game-specific memory offsets for hook targets

### Folder Structure for Game Integration
```
src/games/{GameName}/
├── {GameName}Entry.h          // Mod entry point, inherits from Mod
├── {GameName}Entry.cpp
├── {GameName}RendererModule.h // Frame/render tick hooks
├── {GameName}RendererModule.cpp
├── {GameName}CameraModule.h   // View/projection matrix hooks
├── {GameName}CameraModule.cpp
├── memory/
│   └── offsets.h              // Game-specific pattern/offset definitions
└── sdk/
    └── {GameName}SDK.h        // Reversed/generated engine structs
```

---

## Architecture (Flat)

```
D3D12Hook (vtable/pointer) --> ViewInjector --> FoveatedAtlas (double-height RT)
       |                            |                   |
       v                            v                   v
SetViewports/SetRTs         StereoEmulator(4 views)   VisibilityCache
                                   |
                                   v
                            ISR Draw Pairs:  A(0+1), B(2+3)

Atlas Layout:  [FovealL|FovealR] top, [PeriphL|PeriphR] bottom
```

---

## Core Components

### 1. StereoEmulator
Manages 4 emulated views with projection/frustum data.

```cpp
// src/mods/foveated/StereoEmulator.hpp
#pragma once
#include <array>
#include <glm/glm.hpp>
#include "mods/vr/runtimes/VRRuntime.hpp"

namespace foveated {

enum class ViewType : uint8_t {
    FOVEAL_LEFT_PRIMARY = 0, FOVEAL_RIGHT_SECONDARY = 1,
    PERIPHERAL_LEFT_PRIMARY = 2, PERIPHERAL_RIGHT_SECONDARY = 3
};

struct EmulatedView {
    glm::mat4 projection, view;
    glm::vec4 frustumPlanes[6];
    D3D12_VIEWPORT viewport;
    float fovScale;
    ViewType type;
    uint32_t stereoPassMask; // 0x1=Primary, 0x2=Secondary
};

class StereoEmulator {
public:
    static StereoEmulator& get();
    void initialize(runtimes::VRRuntime* runtime);
    void beginFrame(int frameIndex);
    bool isStereoActive() const { return m_stereoActive; }
    const EmulatedView& getView(ViewType t) const { return m_views[(size_t)t]; }
    std::array<D3D12_VIEWPORT, 4> computeAtlasViewports(uint32_t w, uint32_t h) const;
    void configureFOV(float fovealDeg, float peripheralDeg);
private:
    void buildViewMatrices(int frame);
    void computeFrustumPlanes(EmulatedView& v);
    std::array<EmulatedView, 4> m_views;
    runtimes::VRRuntime* m_runtime = nullptr;
    bool m_stereoActive = false;
    float m_fovealFov = 40.f, m_peripheralFov = 110.f;
    float m_fovealScale = 1.f, m_peripheralScale = 0.5f;
};
} // namespace foveated
```

### 2. ViewInjector
Hooks D3D12 viewport/RT calls to redirect to atlas.

```cpp
// src/mods/foveated/ViewInjector.hpp
#pragma once
#include "StereoEmulator.hpp"
#include <d3d12.h>

namespace foveated {

class ViewInjector {
public:
    static ViewInjector& get();
    void install(class D3D12Hook* hook);
    void setAtlasRT(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv);
private:
    void onSetViewports(ID3D12GraphicsCommandList5* cmd, UINT num, const D3D12_VIEWPORT* vps);
    void onSetRenderTargets(ID3D12GraphicsCommandList5* cmd, UINT num,
        const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, BOOL single, D3D12_CPU_DESCRIPTOR_HANDLE* dsv);
    void injectFovealPair(ID3D12GraphicsCommandList5* cmd);
    void injectPeripheralPair(ID3D12GraphicsCommandList5* cmd);

    D3D12Hook* m_hook = nullptr;
    ID3D12Resource* m_atlasRT = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_atlasRTV{};
    std::array<D3D12_VIEWPORT, 4> m_atlasViewports;
    bool m_isRenderingFoveal = false;
};
} // namespace foveated
```

### 3. FoveatedAtlas
Double-height render target for 4-view output.

```cpp
// src/mods/foveated/FoveatedAtlas.hpp
#pragma once
#include <d3d12.h>
#include <wrl/client.h>

namespace foveated {

struct AtlasConfig {
    uint32_t fovealWidth = 1440, fovealHeight = 1600;
    uint32_t peripheralWidth = 720, peripheralHeight = 800;
    DXGI_FORMAT format = DXGI_FORMAT_R10G10B10A2_UNORM;
};

class FoveatedAtlas {
public:
    static FoveatedAtlas& get();
    bool initialize(ID3D12Device* dev, const AtlasConfig& cfg);
    void shutdown();
    ID3D12Resource* getTexture() const { return m_atlas.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const { return m_rtv; }
    uint32_t getTotalWidth() const { return m_cfg.fovealWidth * 2; }
    uint32_t getTotalHeight() const { return m_cfg.fovealHeight + m_cfg.peripheralHeight; }
    void transitionToRT(ID3D12GraphicsCommandList* cmd);
    void transitionToSRV(ID3D12GraphicsCommandList* cmd);
private:
    AtlasConfig m_cfg;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_atlas, m_depth;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap, m_dsvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv{}, m_dsv{};
    D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_COMMON;
};
} // namespace foveated
```

### 4. VisibilityCache (Mega-Frustum Optimization)
Shares culling results between foveal/peripheral pairs.

```cpp
// src/mods/foveated/VisibilityCache.hpp
#pragma once
#include <array>
#include <vector>
#include <shared_mutex>

namespace foveated {

class VisibilityCache {
public:
    static VisibilityCache& get();
    void initialize(uint32_t maxPrimitives);
    void beginFrame(int frameIdx);
    void recordVisibility(uint32_t viewIdx, const uint8_t* bits, size_t count);
    const uint8_t* getVisibility(uint32_t viewIdx) const;
    bool canShare(uint32_t src, uint32_t dst) const; // 0->2, 1->3
    void copyVisibility(uint32_t src, uint32_t dst);
private:
    static constexpr size_t FRAMES = 3, VIEWS = 4;
    struct Frame { std::array<std::vector<uint8_t>, VIEWS> masks; std::array<bool, VIEWS> valid; };
    std::array<Frame, FRAMES> m_frames;
    int m_current = 0;
    mutable std::shared_mutex m_mtx;
};
} // namespace foveated
```

---

## Example Game Integration (UE-based)

Following `acsvr` pattern for Unreal Engine games:

```cpp
// src/games/ExampleUE/ExampleUEEntry.h
#pragma once
#include "Mod.hpp"

class ExampleUEEntry : public Mod {
public:
    static std::shared_ptr<ExampleUEEntry>& get() {
        static auto inst = std::make_shared<ExampleUEEntry>();
        return inst;
    }
    std::string_view get_name() const override { return "ExampleUE VR"; }
    std::optional<std::string> on_initialize() override;
    void on_draw_ui() override;
private:
    ModSlider::Ptr m_hudScale = ModSlider::create("HudScale", 0.1f, 1.0f, 0.5f);
    ModToggle::Ptr m_decoupledPitch = ModToggle::create("DecoupledPitch", false);
};
```

```cpp
// src/games/ExampleUE/ExampleUEEntry.cpp
#include "ExampleUEEntry.h"
#include "ExampleUERendererModule.h"
#include "ExampleUECameraModule.h"

std::optional<std::string> ExampleUEEntry::on_initialize() {
    ExampleUERendererModule::get()->installHooks();
    ExampleUECameraModule::get()->installHooks();
    return std::nullopt;
}

void ExampleUEEntry::on_draw_ui() {
    if (!ImGui::CollapsingHeader(get_name().data())) return;
    m_hudScale->draw("HUD Scale");
    m_decoupledPitch->draw("Decoupled Pitch");
}
```

```cpp
// src/games/ExampleUE/ExampleUERendererModule.h
#pragma once
#include <safetyhook/inline_hook.hpp>

class ExampleUERendererModule {
public:
    static ExampleUERendererModule* get();
    void installHooks();
private:
    safetyhook::InlineHook m_beginFrameHook{};
    safetyhook::InlineHook m_beginRenderHook{};
    static uintptr_t onBeginFrame();
    static uintptr_t onBeginRender(void* context);
};
```

```cpp
// src/games/ExampleUE/ExampleUERendererModule.cpp
#include "ExampleUERendererModule.h"
#include "memory/offsets.h"
#include <mods/VR.hpp>
#include <Framework.hpp>

ExampleUERendererModule* ExampleUERendererModule::get() {
    static auto inst = new ExampleUERendererModule();
    return inst;
}

void ExampleUERendererModule::installHooks() {
    auto beginFrameFn = memory::beginFrameAddr();
    m_beginFrameHook = safetyhook::create_inline((void*)beginFrameFn, (void*)&onBeginFrame);

    auto beginRenderFn = memory::beginRenderAddr();
    m_beginRenderHook = safetyhook::create_inline((void*)beginRenderFn, (void*)&onBeginRender);
}

uintptr_t ExampleUERendererModule::onBeginFrame() {
    auto inst = get();
    return inst->m_beginFrameHook.call<uintptr_t>();
}

uintptr_t ExampleUERendererModule::onBeginRender(void* ctx) {
    auto inst = get();
    if (g_framework->is_ready()) {
        auto vr = VR::get();
        vr->m_engine_frame_count++;
        g_framework->enable_engine_thread();
        g_framework->run_imgui_frame(false);
        vr->m_render_frame_count = vr->m_engine_frame_count;
        vr->on_begin_rendering(vr->m_render_frame_count);
        vr->update_hmd_state(vr->m_render_frame_count);
        auto result = inst->m_beginRenderHook.call<uintptr_t>(ctx);
        vr->m_presenter_frame_count = vr->m_render_frame_count;
        return result;
    }
    return inst->m_beginRenderHook.call<uintptr_t>(ctx);
}
```

```cpp
// src/games/ExampleUE/ExampleUECameraModule.h
#pragma once
#include <safetyhook/inline_hook.hpp>
#include <glm/glm.hpp>

namespace sdk { struct FMinimalViewInfo; struct APlayerCameraManager; }

class ExampleUECameraModule {
public:
    static ExampleUECameraModule* get();
    void installHooks();
private:
    safetyhook::InlineHook m_calcViewHook{};
    safetyhook::InlineHook m_getProjectionHook{};

    static void onCalcView(sdk::APlayerCameraManager* camMgr, float dt, sdk::FMinimalViewInfo* outView);
    static void onGetProjection(glm::mat4* outProj, float fov, float aspect, float nearZ, float farZ);
};
```

```cpp
// src/games/ExampleUE/ExampleUECameraModule.cpp
#include "ExampleUECameraModule.h"
#include "memory/offsets.h"
#include "aer/ConstantsPool.h"
#include <mods/VR.hpp>

ExampleUECameraModule* ExampleUECameraModule::get() {
    static auto inst = new ExampleUECameraModule();
    return inst;
}

void ExampleUECameraModule::installHooks() {
    auto calcViewFn = memory::calcViewAddr();
    m_calcViewHook = safetyhook::create_inline((void*)calcViewFn, (void*)&onCalcView);

    auto getProjectionFn = memory::getProjectionAddr();
    m_getProjectionHook = safetyhook::create_inline((void*)getProjectionFn, (void*)&onGetProjection);
}

void ExampleUECameraModule::onCalcView(sdk::APlayerCameraManager* camMgr, float dt, sdk::FMinimalViewInfo* outView) {
    auto inst = get();
    inst->m_calcViewHook.call<void>(camMgr, dt, outView);

    auto vr = VR::get();
    if (vr->is_hmd_active()) {
        auto eye = vr->get_current_eye_transform();
        auto hmd = vr->get_transform(0);
        auto offset = vr->get_transform_offset();
        // Apply VR transform to outView->Location/Rotation
    }
}

void ExampleUECameraModule::onGetProjection(glm::mat4* outProj, float fov, float aspect, float nearZ, float farZ) {
    auto inst = get();
    inst->m_getProjectionHook.call<void>(outProj, fov, aspect, nearZ, farZ);

    auto vr = VR::get();
    if (vr->is_hmd_active()) {
        GlobalPool::submit_projection(*outProj, vr->m_render_frame_count);
    }
}
```

```cpp
// src/games/ExampleUE/memory/offsets.h
#pragma once
#include "memory/memory_mul.h"

namespace memory {
    inline uintptr_t beginFrameAddr() {
        return FuncRelocation("BeginFrame", "48 89 5C 24 ?  57 48 83 EC 20 48 8B D9 E8", 0x0);
    }
    inline uintptr_t beginRenderAddr() {
        return FuncRelocation("BeginRender", "48 89 5C 24 ?  48 89 74 24 ?  57 48 83 EC 30", 0x0);
    }
    inline uintptr_t calcViewAddr() {
        return FuncRelocation("CalcView", "40 53 48 83 EC 40 48 8B DA 48 8B D1", 0x0);
    }
    inline uintptr_t getProjectionAddr() {
        return FuncRelocation("GetProjection", "48 83 EC 48 0F 29 74 24 ?", 0x0);
    }
}
```

---

## Integration Checklist

1. **Create game folder**: `src/games/{GameName}/`
2. **Define offsets**: Find via signature scan or static analysis
3. **Implement modules**:
   - `{GameName}Entry` - Mod registration, UI
   - `{GameName}RendererModule` - Frame begin/end hooks
   - `{GameName}CameraModule` - View/projection hooks
4. **Register in Mods.cpp**: Add to mod list
5. **Hook targets** (minimum required):
   - Frame begin (increment frame counters)
   - Render begin (call VR::on_begin_rendering)
   - Projection matrix calc (inject VR projection)
   - View matrix calc (inject HMD transform)

---

## Key Hook Points Summary

| Hook | Purpose | vrframework API |
|------|---------|-----------------|
| BeginFrame | Frame counter sync | `vr->m_engine_frame_count++` |
| BeginRender | VR state update | `vr->on_begin_rendering()`, `vr->update_hmd_state()` |
| CalcProjection | VR projection injection | `GlobalPool::submit_projection()` |
| CalcView | HMD transform injection | `vr->get_transform()`, `vr->get_current_eye_transform()` |
