#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "DriverManager.hpp"
#include "TaskScheduler.hpp"
#include "../Update/Offsets.hpp"
#include "../Update/Globals.hpp"
#include <vector>
#include <spdlog/spdlog.h>




inline HANDLE driver_handle;

namespace Drvr {
	inline std::string readstring(std::uintptr_t address) {
		std::string result;
		result.reserve(204);
		for (int offset = 0; offset < 200; offset++) {
			char character = driver::read_mem<char>(driver_handle, address + offset);
			if (character == 0) break;
			result.push_back(character);
		}
		return result;
	}




	// namespace Drvr
}


namespace Memory {
	static DWORD get_process_id(const wchar_t* process_name) {
		DWORD processId = 0;
		HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (snap_shot == INVALID_HANDLE_VALUE) return processId;

		PROCESSENTRY32W entry = {};
		entry.dwSize = sizeof(entry);
		if (Process32FirstW(snap_shot, &entry)) {
			do {
				if (_wcsicmp(process_name, entry.szExeFile) == 0) {
					processId = entry.th32ProcessID;
					break;
				}
			} while (Process32NextW(snap_shot, &entry));
		}
		CloseHandle(snap_shot);
		return processId;
	}
	static uintptr_t get_module_base(DWORD pid, const wchar_t* module_name) {
		uintptr_t module_base = 0;
		HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (snap_shot == INVALID_HANDLE_VALUE) return module_base;

		MODULEENTRY32W entry = {};
		entry.dwSize = sizeof(entry);
		if (Module32FirstW(snap_shot, &entry)) {
			do {
				if (_wcsicmp(module_name, entry.szModule) == 0) {
					module_base = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
					break;
				}
			} while (Module32NextW(snap_shot, &entry));
		}
		CloseHandle(snap_shot);
		return module_base;
	}
}



class Instance {
public:
	uintptr_t Address;
	Instance() : Address(0) {}

	explicit Instance(uintptr_t address) : Address(address) {}

	[[nodiscard]] std::string GetName() const {
		const auto ptr = driver::read_mem<uintptr_t>(driver_handle, Address + Offsets::name);
		if (ptr)
			return Drvr::readstring(ptr);
		return "";
	}

	[[nodiscard]] Instance GetParent() const {
		return Instance(driver::read_mem<uintptr_t>(driver_handle, Address + Offsets::parent));
	}

	[[nodiscard]] std::vector<Instance> GetChildren() const {
		std::vector<Instance> container;

		auto start = driver::read_mem<uint64_t>(driver_handle, Address + Offsets::children);
		auto end = driver::read_mem<uint64_t>(driver_handle, start + Offsets::size);

		for (auto child = driver::read_mem<uint64_t>(driver_handle, start); child != end; child += 0x10)
			container.emplace_back(driver::read_mem<uint64_t>(driver_handle, child));

		return container;
	}

	[[nodiscard]] Instance FindFirstChild(const std::string& childname) const {
		for (const Instance& child : GetChildren()) {
			if (child.GetName() == childname) {
				return child;
			}
		}
		return Instance(0);
	}

	[[nodiscard]] Instance GetValue() const {
		return Instance(driver::read_mem<uintptr_t>(driver_handle, Address + Offsets::value));
	}

	[[nodiscard]] bool IsValid() const {
		return Address != 0;
	}
};


inline void Setup() {

	// GET PROCESS //
	const DWORD pid = Memory::get_process_id(L"RobloxPlayerBeta.exe");
	if (pid == 0) {
		spdlog::error("Failed to get process ID");
		std::cin.get();
		return;
	}
	spdlog::info("Found Roblox at Process ID: {}", pid);

	// DRIVER SETUP //
	driver_handle = CreateFileW(L"\\\\.\\RobloxDriva", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (driver_handle == INVALID_HANDLE_VALUE) {
		spdlog::error("Failed to open driver handle (ran as admin?)");
		std::cin.get();
		return;
	}
	spdlog::info("Driver handle opened successfully");

	// ATTACH TO PROCESS //
	if (!driver::attachprocess(driver_handle, pid)) {
		spdlog::error("Failed to attach to process");
		std::cin.get();
		return;
	}
	spdlog::info("Attached to process successfully");
}



namespace instance {
	inline void instance() {
		// 
	}
}
namespace actions {
	inline void Walkspeed(int val) {
		spdlog::warn("attempting to set Walkspeed to {}", val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeed, val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeedcheck, val);

		if (driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeed) != val) {
			spdlog::error("Failed to set Walkspeed");
			return;
		}

		spdlog::info("Walkspeed set to {}", val);
	}

	inline void Health(int val) {
		spdlog::warn("attempting to set Health to {}", val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::health, val);
		if (driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::health) != val) {
			spdlog::error("Failed to set Health");
			return;
		}
		spdlog::info("Health set to {}", val);
	}
	inline void JumpPower(int val) {
		spdlog::warn("attempting to set JumpPower to {}", val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::jumppower, val);
		if (driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::jumppower) != val) {
			spdlog::error("Failed to set JumpPower");
			return;
		}
		spdlog::info("JumpPower set to {}", val);
	}

	inline void Gravity(int val) {
		spdlog::warn("attempting to set Gravity to {}", val);
		driver::write_mem<float>(driver_handle, Globals::workspace + Offsets::gravity, val);
		if (driver::read_mem<float>(driver_handle, Globals::workspace + Offsets::gravity) != val) {
			spdlog::error("Failed to set Gravity");
			return;
		}
		spdlog::info("Gravity set to {}", val);
	}

}