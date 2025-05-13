#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>




namespace driver {
	namespace codes {
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct Requests {
		HANDLE process_id;
		PVOID  target;
		PVOID  buffer;
		SIZE_T size;
		SIZE_T return_size;
	};

	inline bool attachprocess(HANDLE driver_handle, DWORD pid) {
		Requests r;
		r.process_id = reinterpret_cast<HANDLE>(pid);
		return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}

	template <typename T>
	T read_mem(HANDLE driver_handle, uintptr_t addr) {
		T temp{};
		Requests r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = &temp;
		r.size = sizeof(T);
		DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
		return temp;
	}

	template <typename T>
	void write_mem(HANDLE driver_handle, uintptr_t addr, const T& value) {
		Requests r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = (PVOID)&value;
		r.size = sizeof(T);
		DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}
}
