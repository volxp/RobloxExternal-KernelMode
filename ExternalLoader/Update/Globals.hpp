#pragma once
#include <Windows.h>
#include <iostream>

namespace Globals {
    inline HANDLE handle = 0;
    inline DWORD pid = 0;
    inline HWND window = 0;
    inline uintptr_t base_address = 0;
    inline uintptr_t guarded_region = 0;
    inline uintptr_t datamodel = 0;
    inline uintptr_t player = 0;
    inline uintptr_t humanoid = 0;
    inline uintptr_t workspace = 0;
    inline uintptr_t camera = 0;

}

namespace memory {
    enum class protection_flags_t {
        readwrite = 0x04,
        execute = 0x10,
        execute_readwrite = 0x20,
    };

    enum class state_t {
        commit_t = 0x1000,
        reserve_t = 0x2000,
        free_t = 0x10000
    };

    struct region_t {
        uintptr_t BaseAddress;
        size_t RegionSize;
        protection_flags_t Protect;
        state_t State;
    };
}
namespace Other {
    inline void PrintAddress(uintptr_t address, const std::string& name) {
        std::cout << name << ": 0x" << std::uppercase << std::hex << address << std::endl;
    }

    inline void PrintString(const std::string& name) {
        std::cout << name << std::endl;
    }
}