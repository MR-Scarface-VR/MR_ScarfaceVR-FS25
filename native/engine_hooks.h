#pragma once
#include <windows.h>
#include <cstring>

inline bool native_perspective(const void* camera) {
    return camera && *(static_cast<const char*>(camera) + 0x168) == 0;
}

inline bool script_registration_ready(const void* engine) {
    return engine && *reinterpret_cast<void* const*>(static_cast<const char*>(engine) + 0x10) &&
           *(static_cast<const char*>(engine) + 0xe0) == 0;
}

inline void** find_import_slot(unsigned char* base, size_t imageSize, const char* name) {
    auto bounded = [imageSize](size_t offset, size_t bytes) {
        return offset <= imageSize && bytes <= imageSize - offset;
    };
    if (!base || !name || !bounded(0, sizeof(IMAGE_DOS_HEADER))) return nullptr;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
        !bounded(static_cast<size_t>(dos->e_lfanew), sizeof(IMAGE_NT_HEADERS64))) return nullptr;
    const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        pe->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_IMPORT) return nullptr;
    const auto directory = pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress || !bounded(directory.VirtualAddress, directory.Size)) return nullptr;
    for (size_t entry = 0; entry + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= directory.Size; entry += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        const auto* descriptor = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress + entry);
        if (!descriptor->Name) break;
        if (!descriptor->OriginalFirstThunk || !descriptor->FirstThunk) continue;
        for (size_t index = 0; ; ++index) {
            const size_t input = static_cast<size_t>(descriptor->OriginalFirstThunk) + index*sizeof(IMAGE_THUNK_DATA64);
            const size_t output = static_cast<size_t>(descriptor->FirstThunk) + index*sizeof(IMAGE_THUNK_DATA64);
            if (!bounded(input, sizeof(IMAGE_THUNK_DATA64)) || !bounded(output, sizeof(IMAGE_THUNK_DATA64))) break;
            const auto* thunk = reinterpret_cast<const IMAGE_THUNK_DATA64*>(base + input);
            if (!thunk->u1.AddressOfData) break;
            if (IMAGE_SNAP_BY_ORDINAL64(thunk->u1.Ordinal)) continue;
            const auto address = static_cast<size_t>(thunk->u1.AddressOfData);
            if (!bounded(address, sizeof(WORD)+1)) break;
            const auto* importName = reinterpret_cast<const char*>(base + address + sizeof(WORD));
            if (!std::memchr(importName, 0, imageSize - address - sizeof(WORD))) break;
            if (std::strcmp(importName, name) == 0) return reinterpret_cast<void**>(base + output);
        }
    }
    return nullptr;
}

inline void* native_swapchain(void* device, void* expectedBackend, void* expectedWindow) {
    if (!device || *static_cast<void**>(device) != expectedBackend) return nullptr;
    const auto window = *reinterpret_cast<char**>(static_cast<char*>(device) + 0x388);
    if (!window || *reinterpret_cast<void**>(window) != expectedWindow) return nullptr;
    return *reinterpret_cast<void**>(window + 0xb0);
}

inline bool replace_slot(void** slot, void* expected, void* replacement) {
    if (!slot || !expected || !replacement) {
        return false;
    }
    DWORD previousProtection;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &previousProtection)) {
        return false;
    }
    void* previous = InterlockedCompareExchangePointer(
        reinterpret_cast<void* volatile*>(slot), replacement, expected);
    DWORD ignored;
    const bool restored = VirtualProtect(slot, sizeof(void*), previousProtection, &ignored) != FALSE;
    return previous == expected && restored;
}
