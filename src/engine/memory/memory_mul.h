#pragma once

namespace memory {
    extern uintptr_t InstructionRelocation(const char* hook_name, const char* pattern, UINT offset_begin, UINT instruction_size,  uintptr_t static_offset);
    extern uintptr_t FuncRelocation(const char* hook_name, const char* pattern, uintptr_t static_offset);
    extern uintptr_t VTable(const char* hook_name, const char* table, uintptr_t static_offset);
    extern bool PatchMemory(uintptr_t address, const std::vector<uint8_t>& patchBytes);
    extern bool PatchMemory(uintptr_t address, const unsigned char* patch, size_t patchSize);

    extern HMODULE g_mod;
    extern size_t mod_size;
    extern size_t mod_end;

    inline uintptr_t disable_taa_addr() {
        static auto pattern = "F3 41 0F 11 42 08 F3 0F 10 ? ? ? ? ? F3 41 0F 11 4A 0C";
        static auto addr = FuncRelocation("disable_taa_addr", pattern, 0x52ece6);
        return addr;
    }

    inline uintptr_t disable_jitter_addr() {
        static auto pattern = "C6 83 A0 03 00 00 01";
        static auto addr = FuncRelocation("disable_jitter_addr", pattern, 0x4d2d18);
        return addr;
    }

    inline uintptr_t disable_dof_addr() {
        static auto pattern = "F3 44 0F 59 C7 F3 0F 59 D7";
        static auto addr = FuncRelocation("disable_dof_addr", pattern, 0x4116da);
        return addr;
    }

    inline uintptr_t disable_sharpness_addr() {
        static auto pattern = "0F 28 C2 C7 05 ? ? ? 00 00 00 80 3F";
        static auto addr = FuncRelocation("disable_sharpness_addr", pattern, 0x412767);
        return addr;
    }

    inline uintptr_t disable_letterbox_addr() {
        static uintptr_t patchAddress = (uintptr_t)memory::g_mod + 0x991695;
        return patchAddress;
    }

    inline uintptr_t camera_server_vftbl_addr() {
        // ".?AVServer@camera@@"
        static uintptr_t addr = (uintptr_t)memory::g_mod + 0xdf9460;
        return addr;
    }
}
