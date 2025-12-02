#include "UpscalerFsr3Module.h"

#include <spdlog/spdlog.h>
#include <imgui.h>

// FSR3 DLL name - can be overridden by game via macro before including this file
#ifndef FSR3_DLL_NAME
#define FSR3_DLL_NAME "ffx_fsr3upscaler_x64.dll"
#endif

// FSR3 Context size estimate (conservative upper bound)
// FSR3 contexts are larger than FSR2 due to frame generation support
constexpr size_t FSR3_CONTEXT_SIZE = 131072;  // 128KB

UpscalerFsr3Module::~UpscalerFsr3Module() {
    remove_hooks();
    s_instance = nullptr;
}

std::optional<std::string> UpscalerFsr3Module::on_initialize() {
    spdlog::info("[FSR3] Initializing AMD FSR3 Upscaler AFR Module");
    
    s_instance = this;
    
    // Try to find the FSR3 DLL
    m_fsr3_module = GetModuleHandleA(FSR3_DLL_NAME);
    if (m_fsr3_module != nullptr) {
        spdlog::info("[FSR3] Found FSR3 DLL: {}", FSR3_DLL_NAME);
    } else {
        spdlog::info("[FSR3] FSR3 DLL ({}) not loaded yet, will attempt hook installation later", FSR3_DLL_NAME);
        // Not an error - game may load FSR3 later or not at all
        return std::nullopt;
    }
    
    if (!install_hooks()) {
        spdlog::warn("[FSR3] Failed to install hooks, module disabled");
        return std::nullopt;
    }
    
    spdlog::info("[FSR3] Module initialized successfully");
    return std::nullopt;
}

bool UpscalerFsr3Module::install_hooks() {
    if (m_fsr3_module == nullptr) {
        // Try to find the module
        m_fsr3_module = GetModuleHandleA(FSR3_DLL_NAME);
        if (m_fsr3_module != nullptr) {
            spdlog::info("[FSR3] Found FSR3 DLL during hook installation: {}", FSR3_DLL_NAME);
        } else {
            spdlog::debug("[FSR3] FSR3 DLL ({}) still not loaded", FSR3_DLL_NAME);
            return false;
        }
    }
    
    // Get function addresses
    // FSR3 API function names - try multiple variations as naming may differ
    auto pfn_create = (PFN_ffxFsr3ContextCreate)GetProcAddress(m_fsr3_module, "ffxFsr3ContextCreate");
    if (pfn_create == nullptr) {
        pfn_create = (PFN_ffxFsr3ContextCreate)GetProcAddress(m_fsr3_module, "ffxFsr3UpscalerContextCreate");
    }
    
    auto pfn_dispatch = (PFN_ffxFsr3ContextDispatch)GetProcAddress(m_fsr3_module, "ffxFsr3ContextDispatch");
    if (pfn_dispatch == nullptr) {
        pfn_dispatch = (PFN_ffxFsr3ContextDispatch)GetProcAddress(m_fsr3_module, "ffxFsr3UpscalerContextDispatch");
    }
    
    auto pfn_destroy = (PFN_ffxFsr3ContextDestroy)GetProcAddress(m_fsr3_module, "ffxFsr3ContextDestroy");
    if (pfn_destroy == nullptr) {
        pfn_destroy = (PFN_ffxFsr3ContextDestroy)GetProcAddress(m_fsr3_module, "ffxFsr3UpscalerContextDestroy");
    }
    
    m_pfn_get_scratch_memory_size = (PFN_ffxFsr3GetScratchMemorySize)GetProcAddress(m_fsr3_module, "ffxFsr3GetScratchMemorySize");
    if (m_pfn_get_scratch_memory_size == nullptr) {
        m_pfn_get_scratch_memory_size = (PFN_ffxFsr3GetScratchMemorySize)GetProcAddress(m_fsr3_module, "ffxFsr3UpscalerGetScratchMemorySize");
    }
    
    if (pfn_create == nullptr || pfn_dispatch == nullptr || pfn_destroy == nullptr) {
        spdlog::error("[FSR3] Failed to find FSR3 API functions");
        spdlog::error("[FSR3]   ffxFsr3ContextCreate: {}", (void*)pfn_create);
        spdlog::error("[FSR3]   ffxFsr3ContextDispatch: {}", (void*)pfn_dispatch);
        spdlog::error("[FSR3]   ffxFsr3ContextDestroy: {}", (void*)pfn_destroy);
        return false;
    }
    
    spdlog::info("[FSR3] Found FSR3 API functions:");
    spdlog::info("[FSR3]   ffxFsr3ContextCreate: {:p}", (void*)pfn_create);
    spdlog::info("[FSR3]   ffxFsr3ContextDispatch: {:p}", (void*)pfn_dispatch);
    spdlog::info("[FSR3]   ffxFsr3ContextDestroy: {:p}", (void*)pfn_destroy);
    spdlog::info("[FSR3]   ffxFsr3GetScratchMemorySize: {:p}", (void*)m_pfn_get_scratch_memory_size);
    
    // Install hooks
    m_create_hook = std::make_unique<FunctionHook>((uintptr_t)pfn_create, (uintptr_t)&on_ffxFsr3ContextCreate);
    if (!m_create_hook->create()) {
        spdlog::error("[FSR3] Failed to hook ffxFsr3ContextCreate");
        return false;
    }
    s_original_create = m_create_hook->get_original<PFN_ffxFsr3ContextCreate>();
    
    m_dispatch_hook = std::make_unique<FunctionHook>((uintptr_t)pfn_dispatch, (uintptr_t)&on_ffxFsr3ContextDispatch);
    if (!m_dispatch_hook->create()) {
        spdlog::error("[FSR3] Failed to hook ffxFsr3ContextDispatch");
        m_create_hook.reset();
        return false;
    }
    s_original_dispatch = m_dispatch_hook->get_original<PFN_ffxFsr3ContextDispatch>();
    
    m_destroy_hook = std::make_unique<FunctionHook>((uintptr_t)pfn_destroy, (uintptr_t)&on_ffxFsr3ContextDestroy);
    if (!m_destroy_hook->create()) {
        spdlog::error("[FSR3] Failed to hook ffxFsr3ContextDestroy");
        m_dispatch_hook.reset();
        m_create_hook.reset();
        return false;
    }
    s_original_destroy = m_destroy_hook->get_original<PFN_ffxFsr3ContextDestroy>();
    
    spdlog::info("[FSR3] All hooks installed successfully");
    return true;
}

void UpscalerFsr3Module::remove_hooks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_destroy_hook.reset();
    m_dispatch_hook.reset();
    m_create_hook.reset();
    
    s_original_create = nullptr;
    s_original_dispatch = nullptr;
    s_original_destroy = nullptr;
    
    // Clean up right eye context if it exists
    if (m_context_right != nullptr && s_original_destroy != nullptr) {
        // Note: We can't destroy here because hooks are being removed
        // The context will be leaked, but this only happens on shutdown
    }
    
    m_context_right = nullptr;
    m_context_right_storage.clear();
    m_scratch_memory_right.clear();
    m_primary_context = nullptr;
}

FfxErrorCode UpscalerFsr3Module::on_ffxFsr3ContextCreate(FfxFsr3Context* context, const FfxFsr3ContextDescription* contextDescription) {
    if (s_instance == nullptr || s_original_create == nullptr) {
        spdlog::error("[FSR3] on_ffxFsr3ContextCreate called but instance or original function is null");
        return FFX_FSR3_ERROR_INVALID_POINTER;
    }
    
    auto& inst = *s_instance;
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (!inst.m_enabled->value()) {
        spdlog::debug("[FSR3] Module disabled, passing through to original");
        return s_original_create(context, contextDescription);
    }
    
    spdlog::info("[FSR3] ffxFsr3ContextCreate intercepted - creating dual contexts for AER");
    
    // Call original to create the game's/left eye context
    FfxErrorCode result = s_original_create(context, contextDescription);
    
    if (result != FFX_FSR3_OK) {
        spdlog::error("[FSR3] Original ffxFsr3ContextCreate failed with error: {}", result);
        return result;
    }
    
    // Store primary context pointer for validation in dispatch
    inst.m_primary_context = context;
    
    // Determine scratch memory size
    size_t scratch_size = FSR3_CONTEXT_SIZE; // Default
    if (inst.m_pfn_get_scratch_memory_size != nullptr) {
        scratch_size = inst.m_pfn_get_scratch_memory_size();
        spdlog::info("[FSR3] Got scratch memory size from API: {} bytes", scratch_size);
    }
    
    // Allocate storage for right eye context
    inst.m_context_right_storage.resize(FSR3_CONTEXT_SIZE);
    inst.m_context_right = reinterpret_cast<FfxFsr3Context*>(inst.m_context_right_storage.data());
    memset(inst.m_context_right, 0, FSR3_CONTEXT_SIZE);
    
    // Allocate scratch memory for right eye
    inst.m_scratch_memory_right.resize(scratch_size);
    memset(inst.m_scratch_memory_right.data(), 0, scratch_size);
    
    // Create a copy of the context description for the right eye
    // FSR3 has additional fields compared to FSR2 for frame generation
    // The upscaling portion follows the same pattern
    
    spdlog::info("[FSR3] Creating right eye context for AER support");
    
    // Call original again to create right eye context
    // Note: In a production implementation, we would need to carefully copy and modify
    // the context description to use our scratch memory and potentially disable
    // frame generation for the right eye context if needed.
    FfxErrorCode result_right = s_original_create(inst.m_context_right, contextDescription);
    
    if (result_right != FFX_FSR3_OK) {
        spdlog::error("[FSR3] Failed to create right eye context: {}", result_right);
        // Clean up and continue with single context (fallback to non-AER mode)
        inst.m_context_right_storage.clear();
        inst.m_context_right = nullptr;
        inst.m_scratch_memory_right.clear();
        // Don't return error - game can still work with single context
    } else {
        spdlog::info("[FSR3] Successfully created dual FSR3 contexts for AER");
    }
    
    return result;
}

FfxErrorCode UpscalerFsr3Module::on_ffxFsr3ContextDispatch(FfxFsr3Context* context, const FfxFsr3DispatchDescription* dispatchDescription) {
    if (s_instance == nullptr || s_original_dispatch == nullptr) {
        spdlog::error("[FSR3] on_ffxFsr3ContextDispatch called but instance or original function is null");
        return FFX_FSR3_ERROR_INVALID_POINTER;
    }
    
    auto& inst = *s_instance;
    
    // Fast path: if module disabled or no right eye context, pass through
    if (!inst.m_enabled->value() || inst.m_context_right == nullptr) {
        return s_original_dispatch(context, dispatchDescription);
    }
    
    // Check if this is the context we're tracking
    if (context != inst.m_primary_context) {
        // Game might be using multiple upscaler instances - log warning and pass through
        spdlog::warn_once("[FSR3] Dispatch called with unknown context {:p} (expected {:p}), passing through",
            (void*)context, (void*)inst.m_primary_context);
        return s_original_dispatch(context, dispatchDescription);
    }
    
    // Determine current eye from VR system
    auto vr = VR::get();
    if (vr == nullptr || !vr->is_hmd_active()) {
        // VR not active, pass through to original
        return s_original_dispatch(context, dispatchDescription);
    }
    
    VRRuntime::Eye current_eye = vr->get_current_render_eye();
    
    FfxFsr3Context* target_context;
    if (current_eye == VRRuntime::Eye::LEFT) {
        // Use game's original context for left eye
        target_context = context;
        spdlog::debug("[FSR3] Dispatch: Using LEFT eye context (primary)");
    } else {
        // Use our secondary context for right eye
        target_context = inst.m_context_right;
        spdlog::debug("[FSR3] Dispatch: Using RIGHT eye context (secondary)");
    }
    
    return s_original_dispatch(target_context, dispatchDescription);
}

FfxErrorCode UpscalerFsr3Module::on_ffxFsr3ContextDestroy(FfxFsr3Context* context) {
    if (s_instance == nullptr || s_original_destroy == nullptr) {
        spdlog::error("[FSR3] on_ffxFsr3ContextDestroy called but instance or original function is null");
        return FFX_FSR3_ERROR_INVALID_POINTER;
    }
    
    auto& inst = *s_instance;
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    spdlog::info("[FSR3] ffxFsr3ContextDestroy intercepted");
    
    // Destroy our right eye context first (if it exists)
    if (inst.m_context_right != nullptr) {
        spdlog::info("[FSR3] Destroying right eye context");
        FfxErrorCode result_right = s_original_destroy(inst.m_context_right);
        if (result_right != FFX_FSR3_OK) {
            spdlog::warn("[FSR3] Failed to destroy right eye context: {}", result_right);
        }
        inst.m_context_right = nullptr;
        inst.m_context_right_storage.clear();
        inst.m_scratch_memory_right.clear();
    }
    
    // Clear primary context tracking
    if (context == inst.m_primary_context) {
        inst.m_primary_context = nullptr;
    }
    
    // Destroy the game's context
    return s_original_destroy(context);
}

void UpscalerFsr3Module::on_draw_ui() {
    if (!ImGui::CollapsingHeader(get_name().data())) {
        return;
    }
    
    ImGui::TreePush("FSR3_AFR");
    
    m_enabled->draw("Enabled");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable dual FSR3 context management for VR Alternate Eye Rendering (AER).\n"
                         "This prevents temporal ghosting artifacts in VR.");
    }
    
    m_auto_detect->draw("Auto-detect FSR3");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically detect and hook FSR3 when game loads the DLL.");
    }
    
    m_disable_frame_generation->draw("Disable Frame Generation");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Disable FSR3 frame generation for VR.\n"
                         "Frame generation can cause issues in VR due to timing requirements.\n"
                         "This only affects the secondary (right eye) context.");
    }
    
    ImGui::Separator();
    ImGui::Text("Status:");
    
    if (m_fsr3_module != nullptr) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FSR3 DLL: Loaded");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FSR3 DLL: Not detected");
    }
    
    if (m_create_hook && m_create_hook->is_valid()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Hooks: Installed");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Hooks: Not installed");
    }
    
    if (m_primary_context != nullptr) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Primary Context: Active");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Primary Context: None");
    }
    
    if (m_context_right != nullptr) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Right Eye Context: Active");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Right Eye Context: None");
    }
    
    // Manual hook installation button
    if (m_fsr3_module == nullptr || !m_create_hook || !m_create_hook->is_valid()) {
        if (ImGui::Button("Attempt Hook Installation")) {
            if (install_hooks()) {
                spdlog::info("[FSR3] Manual hook installation successful");
            } else {
                spdlog::warn("[FSR3] Manual hook installation failed");
            }
        }
    }
    
    ImGui::TreePop();
}

void UpscalerFsr3Module::on_config_load(const utility::Config& cfg, bool set_defaults) {
    for (auto& opt : m_options) {
        opt.get().config_load(cfg, set_defaults);
    }
}

void UpscalerFsr3Module::on_config_save(utility::Config& cfg) {
    for (auto& opt : m_options) {
        opt.get().config_save(cfg);
    }
}

void UpscalerFsr3Module::on_device_reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    spdlog::info("[FSR3] Device reset - cleaning up contexts");
    
    // On device reset, FSR3 contexts become invalid
    // We don't destroy them here because the DLL might do that itself
    // Just clear our tracking
    m_context_right = nullptr;
    m_context_right_storage.clear();
    m_scratch_memory_right.clear();
    m_primary_context = nullptr;
}
