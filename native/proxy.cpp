#include <windows.h>
#include <unknwn.h>
#include <cstdio>
#include <cwchar>
#include "engine_probe.h"

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version,
                                             REFIID iid, void** output, IUnknown* outer);

namespace {
using CreateInput = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, void**, IUnknown*);
INIT_ONCE initialized = INIT_ONCE_STATIC_INIT;
CreateInput systemCreate = nullptr;
DWORD initializationError = ERROR_PROC_NOT_FOUND;

void recordMessage(const char* text) {
    HMODULE self = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&DirectInput8Create), &self)) {
        return;
    }
    wchar_t path[32768];
    DWORD length = GetModuleFileNameW(self, path, 32768);
    if (!length || length >= 32768) {
        return;
    }
    wchar_t* separator = std::wcsrchr(path, L'\\');
    if (!separator || separator - path + 32 >= 32768) {
        return;
    }
    wcscpy_s(separator + 1, 32768 - static_cast<size_t>(separator + 1 - path), L"fs25-mr_scarfacevr.log");
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    SYSTEMTIME now;
    GetSystemTime(&now);
    char message[1400];
    int count = _snprintf_s(message, sizeof(message), _TRUNCATE,
        "%04u-%02u-%02u %02u:%02u:%02u UTC pid=%lu %s\r\n",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
        GetCurrentProcessId(), text);
    if (count > 0) {
        DWORD written;
        WriteFile(file, message, static_cast<DWORD>(count), &written, nullptr);
    }
    CloseHandle(file);
}

BOOL CALLBACK initialize(PINIT_ONCE, PVOID, PVOID*) {
    // First input call runs after DLL loading; never load libraries from DllMain.
    // An unqualified name can resolve to this proxy even with SEARCH_SYSTEM32.
    wchar_t systemPath[32768];
    const UINT length = GetSystemDirectoryW(systemPath, 32768);
    HMODULE systemInput = nullptr;
    if (length && length < 32768 - 13) {
        wcscat_s(systemPath, L"\\dinput8.dll");
        systemInput = LoadLibraryExW(systemPath, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!systemInput) {
            initializationError = GetLastError();
        }
    } else {
        initializationError = length ? ERROR_FILENAME_EXCED_RANGE : GetLastError();
    }
    if (systemInput) {
        systemCreate = reinterpret_cast<CreateInput>(GetProcAddress(systemInput, "DirectInput8Create"));
        if (!systemCreate) {
            initializationError = GetLastError();
            FreeLibrary(systemInput);
        }
    }
    // The real input DLL stays loaded while the game owns interfaces created by it.
    char message[256];
    sprintf_s(message, "MR_ScarfaceVR diagnostic proxy loaded. system_forwarder=%s d3d12=%p dxgi=%p.",
              systemCreate ? "ready" : "failed",
              static_cast<void*>(GetModuleHandleW(L"d3d12.dll")),
              static_cast<void*>(GetModuleHandleW(L"dxgi.dll")));
    recordMessage(message);
    if (systemCreate) start_engine_probe(recordMessage);
    return TRUE;
}
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version,
                                             REFIID iid, void** output, IUnknown* outer) {
    if (!InitOnceExecuteOnce(&initialized, initialize, nullptr, nullptr)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!systemCreate) {
        if (output) {
            *output = nullptr;
        }
        return HRESULT_FROM_WIN32(initializationError ? initializationError : ERROR_PROC_NOT_FOUND);
    }
    return systemCreate(instance, version, iid, output, outer);
}
