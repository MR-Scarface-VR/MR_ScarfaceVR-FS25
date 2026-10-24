#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <cstdio>

int wmain() {
    wchar_t expectedPath[32768];
    const DWORD pathLength = GetFullPathNameW(L"dinput8.dll", 32768, expectedPath, nullptr);
    if (!pathLength || pathLength >= 32768) {
        std::puts("FAIL: cannot resolve own proxy path");
        return 1;
    }
    HMODULE proxy = LoadLibraryExW(expectedPath, nullptr,
                                   LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!proxy) {
        std::printf("FAIL: own proxy could not be loaded (Win32 %lu)\n", GetLastError());
        return 1;
    }
    using CreateInput = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, void**, IUnknown*);
    auto create = reinterpret_cast<CreateInput>(GetProcAddress(proxy, "DirectInput8Create"));
    if (!create) {
        std::puts("FAIL: DirectInput8Create export missing");
        FreeLibrary(proxy);
        return 1;
    }
    IDirectInput8W* input = nullptr;
    const HRESULT result = create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                                  IID_IDirectInput8W, reinterpret_cast<void**>(&input), nullptr);
    if (FAILED(result) || !input) {
        std::printf("FAIL: actual system DirectInput8 creation failed (%08lx)\n",
                    static_cast<unsigned long>(result));
        FreeLibrary(proxy);
        return 1;
    }
    input->Release();
    FreeLibrary(proxy);
    std::puts("PASS: own proxy forwarded a real DirectInput8 interface");
    return 0;
}
