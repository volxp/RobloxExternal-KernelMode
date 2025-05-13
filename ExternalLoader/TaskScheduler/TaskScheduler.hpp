#pragma once


#include <Windows.h>
#include <psapi.h>
#include <string>
#include <iostream>
#include "../Dependencies/ntdll/ntdll.h"
#include "../Update/Globals.hpp"
#include "../Update/Offsets.hpp"
#include <DbgHelp.h>
#include <fstream>
#include <memory>
#include <vector>
#include <thread>
#include <filesystem>
#include <regex>
#include <unordered_set>
#include <optional>
#include <functional>
#include "TlHelp32.h"
#include <chrono>
#include <spdlog/spdlog.h>
#pragma comment(lib, "dbghelp.lib")




struct memory_factory_t {
    template<typename T>
    T read(uintptr_t addr) {
        T val{};
        ReadProcessMemory(process, (LPCVOID)addr, &val, sizeof(T), nullptr);
        return val;
    }

    template<typename T>
    void write(uintptr_t addr, const T& val) {
        WriteProcessMemory(process, (LPVOID)addr, &val, sizeof(T), nullptr);
    }

    std::unique_ptr<uint8_t[]> read(uintptr_t addr, size_t size) {
        auto buf = std::make_unique<uint8_t[]>(size);
        if (!ReadProcessMemory(process, (LPCVOID)addr, buf.get(), size, nullptr)) return nullptr;
        return buf;
    }

    std::vector<MEMORY_BASIC_INFORMATION> regions() {
        std::vector<MEMORY_BASIC_INFORMATION> result;
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t addr = 0;
        while (VirtualQueryEx(process, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && (mbi.Type == MEM_PRIVATE || mbi.Type == MEM_IMAGE)) {
                result.push_back(mbi);
            }
            addr += mbi.RegionSize;
        }
        return result;
    }

    HANDLE process;
};

struct module_t {
    uintptr_t base;
    size_t size;

    bool contains(uintptr_t addr) const {
        return addr >= base && addr < base + size;
    }
};
struct process_t {
    static std::unique_ptr<process_t> open(const std::string& name) {
        DWORD pid = 0;
        PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32) };
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE && Process32First(snap, &entry)) {
            do {
                if (name == entry.szExeFile) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snap, &entry));
            CloseHandle(snap);
        }
        if (!pid) return nullptr;
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProc) return nullptr;

        auto proc = std::make_unique<process_t>();
        proc->memory_factory.process = hProc;
        proc->handle = hProc;
        proc->pid = pid;
        return proc;
    }

    ~process_t() {
        if (handle) CloseHandle(handle);
    }

    std::vector<module_t> modules() {
        std::vector<module_t> result;
        MODULEENTRY32 mod = { sizeof(MODULEENTRY32) };
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap != INVALID_HANDLE_VALUE && Module32First(snap, &mod)) {
            do {
                result.push_back({ (uintptr_t)mod.modBaseAddr, (size_t)mod.modBaseSize });
            } while (Module32Next(snap, &mod));
            CloseHandle(snap);
        }
        return result;
    }

    HANDLE handle;
    DWORD pid;
    memory_factory_t memory_factory;
};

struct RTTICompleteObjectLocator {
    DWORD signature;
    DWORD offset;
    DWORD cdOffset;
    DWORD typeDescriptor;
    DWORD classDescriptor;
    DWORD baseOffset;
};

struct TypeDescriptor {
    void* vtable;
    uint64_t ptr;
    char name[255];
};


namespace TaskScheduler {
    namespace Datamodel {
        static std::optional<std::string> getLatestLogFile(const std::string& folderPath) {
            std::optional<std::string> latestFile;
            std::filesystem::file_time_type latestTime;

            try {
                for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".log") {
                        auto currentFileTime = std::filesystem::last_write_time(entry);

                        if (!latestFile || currentFileTime > latestTime) {
                            latestFile = entry.path().string();
                            latestTime = currentFileTime;
                        }
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                return std::nullopt;
            }
            catch (const std::exception& e) {
                return std::nullopt;
            }

            return latestFile;
        }

        static std::string readFile(const std::string& filePath) {
            std::ifstream file(filePath);
            if (!file) {
                return "";
            }

            std::ostringstream oss;
            oss << file.rdbuf();
            return oss.str();
        }


        static uintptr_t locateTid(const std::string& content) {
            std::regex pattern(R"(::replaceDataModel: \(stage:\d+, window = 0x[a-zA-Z\d]+\) \[tid:(0x[a-zA-Z\d]+)\])");

            std::smatch matches;
            std::string::const_iterator searchStart(content.cbegin());

            uintptr_t result = 0;
            while (std::regex_search(searchStart, content.cend(), matches, pattern)) {
                result = std::stoull(matches[1], nullptr, 16);
                searchStart = matches.suffix().first;
            }

            return result;
        }

        static std::string demangleSymbol(const std::string& kMangledName) {
            std::string demangledName = std::string(1024, '\0');
            std::string mangledName = kMangledName;

            if (mangledName.starts_with(".?AV")) {
                mangledName = "?" + mangledName.substr(4);
            }

            DWORD Length = UnDecorateSymbolName(mangledName.c_str(), demangledName.data(), demangledName.capacity(), UNDNAME_COMPLETE);
            if (!Length) {
                return mangledName;
            }
            demangledName.resize(Length);

            if (demangledName.starts_with(" ??")) {
                demangledName = demangledName.substr(4);
            }

            return demangledName;
        }

        static uintptr_t getModuleContaining(std::unique_ptr<process_t>& process, uintptr_t address) {
            for (const auto& module : process->modules()) {
                if (module.contains(address)) {
                    return module.base;
                }
            }
            return 0;
        }

        static bool isValidAddress(std::unique_ptr<process_t>& process, uintptr_t address) {
            auto buffer = process->memory_factory.read(address, 0x1);
            if (buffer == nullptr) {
                return false;
            }
            return true;
        }

        static std::optional<std::string> getRTTIName(std::unique_ptr<process_t>& process, uintptr_t objectAddress) {
            uintptr_t vtableAddress = process->memory_factory.read<uintptr_t>(objectAddress);
            if (!vtableAddress) {
                return std::nullopt;
            }

            if (!isValidAddress(process, vtableAddress - sizeof(uintptr_t))) {
                return std::nullopt;
            }

            uintptr_t colAddress = process->memory_factory.read<uintptr_t>(vtableAddress - sizeof(uintptr_t));
            if (!colAddress) {
                return std::nullopt;
            }

            if (!isValidAddress(process, colAddress)) {
                return std::nullopt;
            }

            RTTICompleteObjectLocator col = process->memory_factory.read<RTTICompleteObjectLocator>(colAddress);
            uintptr_t TypeInfoAddress = 0;
            TypeInfoAddress = col.typeDescriptor;
            TypeInfoAddress += getModuleContaining(process, colAddress);


            if (!isValidAddress(process, TypeInfoAddress)) {
                return std::nullopt;
            }

            TypeDescriptor typeInfo = process->memory_factory.read<TypeDescriptor>(TypeInfoAddress);
            return demangleSymbol(typeInfo.name);
        }
        static void recursivePointerWalk(std::unique_ptr<process_t>& process, uintptr_t address, size_t maxOffset, std::function<bool(uintptr_t, uintptr_t)> callback, std::optional<std::unordered_set<uintptr_t>> _cache, uintptr_t depth = 0) {
            std::unordered_set<uintptr_t> cache = _cache.value_or(std::unordered_set<uintptr_t>());

            if (cache.contains(address)) {
                return;
            }

            for (size_t offset = 0; offset < maxOffset; offset += 8) {
                if (!isValidAddress(process, address + offset)) {
                    continue;
                }

                uintptr_t pointer = process->memory_factory.read<uintptr_t>(address + offset);

                if (!isValidAddress(process, pointer)) {
                    continue;
                }

                if (!callback(pointer, depth)) {
                    return;
                }

                recursivePointerWalk(process, pointer, 0x200, callback, cache, depth + 1);

                cache.emplace(pointer);
            }
        }

        static uintptr_t getFirstAncestor(std::unique_ptr<process_t>& process, uintptr_t address) {
            uintptr_t previousObject = 0;
            uintptr_t currentObject = process->memory_factory.read<uintptr_t>(address + 0x50);

            while (currentObject != 0) {
                previousObject = currentObject;
                currentObject = process->memory_factory.read<uintptr_t>(currentObject + 0x50);
            }

            return previousObject;
        }

        static uintptr_t findDatamodelPointer(std::unique_ptr<process_t>& process, uintptr_t tid) {
            uintptr_t dataModel = 0;

            recursivePointerWalk(process, tid, 22160, [&](uintptr_t address, uintptr_t depth) -> bool {
                if (dataModel) {
                    return false;
                }

                auto rttiName = getRTTIName(process, address);
                if (rttiName.has_value()) {
                    std::string& name = rttiName.value();

                    if (name == "RBX::ModuleScript" || name == "RBX::LocalScript" || name == "RBX::Folder") {
                        uintptr_t ancestor = getFirstAncestor(process, address);

                        auto ancestorRtti = getRTTIName(process, ancestor);
                        if (!ancestorRtti.has_value()) {
                            return true;
                        }

                        if (ancestorRtti.value() == "RBX::DataModel") {
                            dataModel = ancestor;
                            return false;
                        }
                    }
                }

                return (depth <= 5);
                }, std::nullopt);

            if (!dataModel) {
                return findDatamodelPointer(process, tid);
            }

            return dataModel;
        }

        static std::string logPath = std::string(getenv("LOCALAPPDATA")) + "\\Roblox\\logs";


        namespace evaluate {
            inline uintptr_t GetDataModel() {
                std::unique_ptr<process_t> process = nullptr;
                do {
                    process = process_t::open("RobloxPlayerBeta.exe");

                    if (!process) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                } while (!process);

                std::optional<std::string> logFile = getLatestLogFile(logPath);
                if (!logFile.has_value()) {
                    return 1;
                } 
				//spdlog::warn("Datamodel: 0x{}", findDatamodelPointer(process, locateTid(readFile(logFile.value()))));
                return findDatamodelPointer(process, locateTid(readFile(logFile.value())));
            }
        }



    }
    


}