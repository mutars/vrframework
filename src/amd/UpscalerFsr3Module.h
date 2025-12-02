#pragma once

#include <Mod.hpp>
#include <memory/FunctionHook.h>
#include <mods/VR.hpp>

#include <vector>
#include <mutex>
#include <d3d12.h>

// Forward declarations for FSR3 types
// We define minimal struct layouts to avoid including the full SDK
// These match the FFX SDK structures for hooking purposes

struct FfxFsr3Context;
struct FfxFsr3ContextDescription;
struct FfxFsr3DispatchDescription;

// FSR3 error codes (same as FSR2)
typedef int32_t FfxErrorCode;
constexpr FfxErrorCode FFX_FSR3_OK = 0;
constexpr FfxErrorCode FFX_FSR3_ERROR_INVALID_POINTER = 0x80000000;
constexpr FfxErrorCode FFX_FSR3_ERROR_INVALID_ARGUMENT = 0x80000001;

// FSR3 function signatures
typedef FfxErrorCode(*PFN_ffxFsr3ContextCreate)(FfxFsr3Context* context, const FfxFsr3ContextDescription* contextDescription);
typedef FfxErrorCode(*PFN_ffxFsr3ContextDispatch)(FfxFsr3Context* context, const FfxFsr3DispatchDescription* dispatchDescription);
typedef FfxErrorCode(*PFN_ffxFsr3ContextDestroy)(FfxFsr3Context* context);
typedef size_t(*PFN_ffxFsr3GetScratchMemorySize)(void);

/**
 * @brief FSR 3 Upscaler Module for VR Alternate Eye Rendering (AER) support
 * 
 * This module implements a "Pass-through + 1" strategy:
 * - Allows the game to manage the Primary Context (Left Eye)
 * - Secretly manages a Secondary Context (Right Eye)
 * - Prevents temporal history corruption in AER scenarios
 * 
 * FSR3 includes frame generation capabilities in addition to upscaling.
 * This module focuses on the upscaling portion for VR compatibility.
 * 
 * The module hooks FSR3 API functions (ffxFsr3ContextCreate, ffxFsr3ContextDispatch, 
 * ffxFsr3ContextDestroy) to intercept calls and route them to the appropriate
 * context based on the current VR eye being rendered.
 */
class UpscalerFsr3Module : public Mod {
public:
    inline static std::shared_ptr<UpscalerFsr3Module>& Get() {
        static auto instance = std::make_shared<UpscalerFsr3Module>();
        return instance;
    }

    // Mod interface implementation
    std::string_view get_name() const override { return "AMD FSR3 Upscaler AFR"; }
    
    std::optional<std::string> on_initialize() override;
    void on_draw_ui() override;
    void on_config_load(const utility::Config& cfg, bool set_defaults) override;
    void on_config_save(utility::Config& cfg) override;
    void on_device_reset() override;

    UpscalerFsr3Module() = default;
    ~UpscalerFsr3Module() override;

private:
    // Hook installation
    bool install_hooks();
    void remove_hooks();

    // Hook callbacks
    static FfxErrorCode on_ffxFsr3ContextCreate(FfxFsr3Context* context, const FfxFsr3ContextDescription* contextDescription);
    static FfxErrorCode on_ffxFsr3ContextDispatch(FfxFsr3Context* context, const FfxFsr3DispatchDescription* dispatchDescription);
    static FfxErrorCode on_ffxFsr3ContextDestroy(FfxFsr3Context* context);

    // State management
    std::mutex m_mutex;
    
    // Right eye context storage
    // We store raw bytes to avoid needing the full FSR3 context structure definition
    std::vector<uint8_t> m_context_right_storage;
    FfxFsr3Context* m_context_right{nullptr};
    
    // Scratch memory for the right eye context
    std::vector<uint8_t> m_scratch_memory_right;
    
    // Track the game's primary context pointer for validation
    FfxFsr3Context* m_primary_context{nullptr};
    
    // Module handle for FSR3 DLL
    HMODULE m_fsr3_module{nullptr};
    
    // Function pointers
    PFN_ffxFsr3GetScratchMemorySize m_pfn_get_scratch_memory_size{nullptr};
    
    // Hooks
    std::unique_ptr<FunctionHook> m_create_hook;
    std::unique_ptr<FunctionHook> m_dispatch_hook;
    std::unique_ptr<FunctionHook> m_destroy_hook;
    
    // Original function pointers (set by hooks)
    inline static PFN_ffxFsr3ContextCreate s_original_create{nullptr};
    inline static PFN_ffxFsr3ContextDispatch s_original_dispatch{nullptr};
    inline static PFN_ffxFsr3ContextDestroy s_original_destroy{nullptr};
    
    // Singleton instance for static callbacks
    inline static UpscalerFsr3Module* s_instance{nullptr};

    // ModValue settings
    const ModToggle::Ptr m_enabled{ ModToggle::create(generate_name("Enabled"), true) };
    const ModToggle::Ptr m_auto_detect{ ModToggle::create(generate_name("AutoDetect"), true) };
    const ModToggle::Ptr m_disable_frame_generation{ ModToggle::create(generate_name("DisableFrameGen"), false) };
    
    ValueList m_options{
        *m_enabled,
        *m_auto_detect,
        *m_disable_frame_generation
    };
};
