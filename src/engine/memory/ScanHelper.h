#pragma once
#include <utility/Module.hpp>
#include <utility/RTTI.hpp>
#include <utility/Scan.hpp>

namespace memory {
    const auto mod      = utility::get_executable();
    const size_t mod_size = utility::get_module_size(mod).value_or(0);
    const auto mod_end  = (uintptr_t)mod + mod_size - 0x100;

    inline uintptr_t VTable(const char* hook_name, const char* table, uintptr_t static_offset)
    {
        uintptr_t val = 0;
#if defined _DEBUG || defined SIGNATURE_SCAN
        auto ref = utility::rtti::find_vtable(mod, table);
        if (!ref) {
            spdlog::error("VTable pattern not found for id={}", hook_name );
            return 0;
        }
        val = ref.value();
        if(static_offset > 0) {
            auto offset = val - (uintptr_t)mod;
            if(offset != static_offset) {
                spdlog::info("FuncRelocation offset does not match static offset for id={} offset={:x} reference={:x}", hook_name, offset, static_offset);
            }
        }
#else
        spdlog::info("Using static offset for VTable id={}", hook_name);
        val =  static_offset + (uintptr_t)mod;
#endif
        return val;
    };

    inline uintptr_t FuncRelocation(const char* hook_name, const char* pattern, uintptr_t static_offset) {
        uintptr_t val = 0;
#if defined _DEBUG || defined SIGNATURE_SCAN
        auto ref = utility::scan(mod, pattern);
        if (!ref) {
            spdlog::error("FuncRelocation pattern not found for id={}", hook_name );
            return 0;
        }
        val = ref.value();
        if(static_offset > 0) {
            auto offset = val - (uintptr_t)mod;
            if(offset != static_offset) {
                spdlog::info("FuncRelocation offset does not match static offset for id={} offset={:x} reference={:x}", hook_name, offset, static_offset);
            }
        }
#else
        spdlog::info("Using static offset for VTable id={}", hook_name);
        val =  static_offset + (uintptr_t)mod;
#endif
        return val;
    };
    inline uintptr_t InstructionRelocation(const char* hook_name, const char* pattern, UINT offset_begin, UINT instruction_size,  uintptr_t static_offset) {
        uintptr_t val = 0;
#if defined _DEBUG || defined SIGNATURE_SCAN
        auto ref = utility::scan(mod, pattern);
        if (!ref) {
            spdlog::error("AsmCodeRelocation pattern not found for id={}", hook_name);
            return 0;
        }
        auto addr = ref.value();
        val = addr + *(int32_t*)(addr + offset_begin) + instruction_size;
        if(static_offset > 0) {
            auto offset = val - (uintptr_t)mod;
            if(offset != static_offset) {
                spdlog::info("AsmCodeRelocation offset does not match static offset for id={} offset={:x} reference={:x}", hook_name, offset, static_offset);
            }
        }
#else
        spdlog::info("Using static offset for VTable id={}", hook_name);
        val = static_offset + (uintptr_t)mod;
#endif
        return val;
    };
}