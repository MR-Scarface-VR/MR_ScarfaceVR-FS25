#include "engine_probe.h"
#include "engine_hooks.h"
#include "stereo.h"
#include "openxr_bridge.h"
#include "engine_addresses.h"
#include "terrain_camera.h"
#include <wrl/client.h>
#include <bcrypt.h>
#include <array>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {
using Render = void(*)(void*);
using Update = void(*)(void*, float);
using BindView = void(*)(void*, void*, unsigned int, unsigned int);
using AfterEye = void(*)(void*, void*, unsigned int);
Render originalRender = nullptr;
Update originalUpdate = nullptr;
BindView originalBind = nullptr;
AfterEye originalAfterEye = nullptr;
void (*record)(const char*) = nullptr;
std::atomic<bool> active = false;
std::atomic<unsigned long long> updates = 0;
std::atomic<unsigned long long> renders = 0;
std::atomic<unsigned int> completedEyes = 0;
std::atomic<unsigned int> positionedEyes = 0;
std::atomic<bool> stereoEnabled = false;
bool keyWasDown = false;
bool recenterWasDown = false;
bool vrWasDown = false;
bool vrEnabled = false;
bool outputEnabled = false;
bool xrDrawing = false;
std::atomic<bool> worldMode = false;
using Present = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
Present originalPresent = nullptr;
IDXGISwapChain* presentSelf = nullptr;
Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain;
DWORD renderThread = 0;
bool luaRegistered = false;
void* registeredLuaState = nullptr;
std::atomic<DWORD> scriptThread = 0;
using Counter = BOOL(WINAPI*)(LARGE_INTEGER*);
Counter originalCounter = nullptr;
void register_lua();
void log(const char* format, ...);
using GetTerrainCamera = void(*)(void*, float, void*, TerrainCameraInfo*);
GetTerrainCamera originalTerrainCamera = nullptr;
TerrainTrackingState terrainTracking;

void terrain_camera(void* camera, float aspect, void* terrain, TerrainCameraInfo* output) {
    originalTerrainCamera(camera, aspect, terrain, output);
    if (!output || !worldMode.load() || !terrainTracking.apply(*output, GetTickCount64())) return;
    static std::atomic<unsigned> samples = 0;
    const auto sample = samples.fetch_add(1) + 1;
    if (sample <= 3 || sample % 600 == 0) {
        log("VR terrain streaming camera: sample=%u thread=%lu forward=(%.3f,%.3f,%.3f) fov=(%.3f,%.3f); base camera unchanged.",
            sample, GetCurrentThreadId(), output->forward.x, output->forward.y, output->forward.z, output->fovX, output->fovY);
    }
}

void publish_terrain_tracking() {
    if (!xrDrawing || !worldMode.load()) { terrainTracking.clear(); return; }
    float tanX = 0, tanY = 0;
    for (unsigned eye = 0; eye < 2; ++eye) {
        const auto fov = vr_eye_fov(eye);
        tanX = (std::max)({tanX, std::abs(std::tan(fov.angleLeft)), std::abs(std::tan(fov.angleRight))});
        tanY = (std::max)({tanY, std::abs(std::tan(fov.angleUp)), std::abs(std::tan(fov.angleDown))});
    }
    terrainTracking.publish(vr_head_pose(), tanX, tanY, GetTickCount64());
}

BOOL WINAPI counter(LARGE_INTEGER* value) {
    const BOOL result = originalCounter(value);
    // The main thread's existing timing boundary also occurs while the VM is resumed.
    static thread_local bool registering = false;
    if (GetCurrentThreadId() == scriptThread.load() && !registering && !luaRegistered) {
        registering = true;
        register_lua();
        registering = false;
    }
    return result;
}

void log(const char* format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    const int length = vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);
    va_end(args);
    if (length > 0 && record) {
        record(message);
    }
}

bool approved_game_file() {
    wchar_t path[32768];
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    if (!length || length >= 32768) {
        return false;
    }
    const wchar_t* name = std::wcsrchr(path, L'\\');
    if (!name || _wcsicmp(name + 1, L"FarmingSimulator2025Game.exe") != 0) {
        return false;
    }
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool valid = false;
    do {
        LARGE_INTEGER size;
        if (!GetFileSizeEx(file, &size) || size.QuadPart != 19370144) {
            break;
        }
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
            BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
            break;
        }
        unsigned char buffer[65536];
        DWORD received = 0;
        bool readOk = true;
        for (;;) {
            if (!ReadFile(file, buffer, sizeof(buffer), &received, nullptr)) {
                readOk = false;
                break;
            }
            if (!received) {
                break;
            }
            if (BCryptHashData(hash, buffer, received, 0) < 0) {
                readOk = false;
                break;
            }
        }
        std::array<unsigned char, 32> digest;
        if (!readOk || BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0) {
            break;
        }
        constexpr std::array<unsigned char, 32> expected = {
            0x90,0xf4,0x1a,0xd5,0xca,0x12,0x55,0x61,0x21,0xaa,0x98,0x84,0x74,0x58,0xc5,0x99,
            0x3f,0x57,0x64,0xac,0x46,0xa9,0x3f,0xfa,0x8d,0x71,0x07,0x80,0xbb,0x5a,0xe9,0x18};
        valid = digest == expected;
    } while (false);
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return valid;
}

unsigned int view_count(void*) {
    return active.load() ? 2u : 1u;
}

unsigned int view_width(void* view, unsigned int) {
    const auto width = *reinterpret_cast<unsigned int*>(static_cast<char*>(view) + 8);
    return active.load() && !xrDrawing ? width / 2u : width;
}

void eye_offset(void*, unsigned int eye, Vector3* output) {
    if (!output) return;
    *output = active.load() ? (xrDrawing ? vr_eye_offset(eye) : diagnostic_eye_offset(eye)) : Vector3{};
    if (active.load() && eye < 2) {
        positionedEyes.fetch_or(1u << eye);
    }
}

void head_translation(void*, Vector3* output) {
    if (output) *output = xrDrawing ? vr_head_pose().position : Vector3{};
}

void head_rotation(void*, float* output) {
    if (output) rotation_matrix(xrDrawing ? vr_head_pose().orientation : Quaternion{0,0,0,1}, output);
}

void projection(void*, unsigned eye, char* orthographic, float* left, float* right, float* bottom, float* top) {
    if (xrDrawing && orthographic && *orthographic) {
        vr_suppress_frame();
        xrDrawing = false;
        active.store(false);
        return;
    }
    if (!xrDrawing || eye > 1 || !orthographic) return;
    const auto fov = vr_eye_fov(eye);
    *left = std::tan(fov.angleLeft);
    *right = std::tan(fov.angleRight);
    *bottom = std::tan(fov.angleDown);
    *top = std::tan(fov.angleUp);
}

HRESULT WINAPI present(IDXGISwapChain* self, UINT interval, UINT flags) {
    if (self == presentSelf && GetCurrentThreadId() == renderThread && !(flags & DXGI_PRESENT_TEST)) {
        // Native window::present flushes the graphics context before this COM call.
        vr_capture_tick(true);
        vr_finish_frame();
    }
    return originalPresent(self, interval, flags);
}

bool connect_swapchain(void* device) {
    auto base = reinterpret_cast<char*>(GetModuleHandleW(nullptr));
    auto raw = static_cast<IDXGISwapChain*>(native_swapchain(device, base + 0xf85a00, base + 0xf8f930));
    if (swapchain) {
        if (raw != presentSelf) {
            // Unconfirmed old command submission prevents safe image retirement/rebinding.
            vr_fail_closed("OpenXR disabled until restart: native swapchain identity changed; old images retained, no stale copies submitted.");
            return false;
        }
        return true;
    }
    if (!raw || FAILED(raw->QueryInterface(IID_PPV_ARGS(&swapchain)))) return false;
    auto table = *reinterpret_cast<void***>(raw);
    originalPresent = reinterpret_cast<Present>(table[8]);
    presentSelf = raw;
    if (!replace_slot(&table[8], reinterpret_cast<void*>(originalPresent), reinterpret_cast<void*>(&present))) {
        swapchain.Reset();
        presentSelf = nullptr;
        return false;
    }
    renderThread = GetCurrentThreadId();
    log("Game D3D12 swapchain connected; eye release is deferred to Present after native command submission.");
    return true;
}

int lua_world(void*) { worldMode.store(true); return 0; }
int lua_menu(void*) { worldMode.store(false); terrainTracking.clear(); return 0; }

void register_lua() {
    if (luaRegistered) return;
    auto base = reinterpret_cast<char*>(GetModuleHandleW(nullptr));
    const unsigned char expectedGetter[] = {0x48, 0x8b, 0x01, 0xc3};
    if (std::memcmp(base + engine_address::scriptGetter, expectedGetter, sizeof(expectedGetter)) != 0) {
        luaRegistered = true;
        log("MR_ScarfaceVR Lua callbacks disabled: script getter instruction guard differs.");
        return;
    }
    using GetEngine = void*(*)(void*);
    auto engine = reinterpret_cast<GetEngine>(base + engine_address::scriptGetter)(base + engine_address::scriptGlobal);
    if (!script_registration_ready(engine)) return;
    using Register = void(*)(void*, int(*)(void*), const char*);
    auto add = reinterpret_cast<Register>(base + engine_address::registerFunction);
    add(engine, lua_world, "ownVRWorld");
    add(engine, lua_menu, "ownVRMenu");
    registeredLuaState = *reinterpret_cast<void**>(static_cast<char*>(engine) + 0x10);
    luaRegistered = true;
    log("MR_ScarfaceVR no-argument Lua mode callbacks registered through the native script engine.");
}

void bind_view(void* view, void* device, unsigned int eye, unsigned int target) {
    if (!active.load() || xrDrawing || eye > 1) {
        originalBind(view, device, eye, target);
        return;
    }
    auto deviceTable = *static_cast<void***>(device);
    using GetContext = void*(*)(void*);
    void* context = reinterpret_cast<GetContext>(deviceTable[0x148 / 8])(device);
    auto contextTable = *static_cast<void***>(context);
    using BindTarget = void(*)(void*, unsigned int, const float*, unsigned int,
                               unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
    const unsigned int width = *reinterpret_cast<unsigned int*>(static_cast<char*>(view) + 8);
    const unsigned int height = *reinterpret_cast<unsigned int*>(static_cast<char*>(view) + 12);
    const unsigned int half = width / 2;
    const float clear[4] = {0, 0, 0, 1};
    // Same native call as the default view, with a separate desktop viewport per eye.
    reinterpret_cast<BindTarget>(contextTable[0x20 / 8])(
        context, target, clear, 0, 0, eye * half, 0,
        eye == 0 ? half : width - half, height);
}

void after_eye(void* view, void* device, unsigned int eye) {
    originalAfterEye(view, device, eye);
    static unsigned debugCalls = 0;
    if (++debugCalls <= 3) log("Native after-eye callback eye=%u XR/capture_active=%u.", eye, vr_frame_active() ? 1u : 0u);
    if (vr_frame_active()) vr_copy_eye(device, eye);
    if (active.load() && eye < 2) {
        completedEyes.fetch_or(1u << eye);
    }
}

void update(void* application, float elapsed) {
    scriptThread.store(GetCurrentThreadId());
    updates.fetch_add(1);
    originalUpdate(application, elapsed);
}

void render(void* application) {
    if (luaRegistered && registeredLuaState) {
        auto base = reinterpret_cast<char*>(GetModuleHandleW(nullptr));
        auto engine = *reinterpret_cast<char**>(base + engine_address::scriptGlobal);
        if (!engine || *reinterpret_cast<void**>(engine + 0x10) != registeredLuaState) {
            luaRegistered = false;
            registeredLuaState = nullptr;
            worldMode.store(false);
        }
    }
    register_lua();
    const bool keyDown = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (keyDown && !keyWasDown) {
        stereoEnabled.store(!stereoEnabled.load());
        log("Desktop stereo diagnostic %s (F9).", stereoEnabled.load() ? "enabled" : "disabled");
    }
    keyWasDown = keyDown;
    const bool recenterDown = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    if (recenterDown && !recenterWasDown) vr_recenter();
    recenterWasDown = recenterDown;
    const bool vrDown = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
    if (vrDown && !vrWasDown) {
#ifndef FS25VR_DESKTOP_CAPTURE
        outputEnabled = !outputEnabled;
        vrEnabled = outputEnabled;
        vr_allow_output(outputEnabled);
        if (!vrEnabled) vr_request_stop();
        log("OpenXR %s (F11); default off.", outputEnabled ? "enabled" : "stopping");
#endif
    }
    vrWasDown = vrDown;
    const auto frame = renders.fetch_add(1) + 1;
    const auto before = updates.load();
    completedEyes.store(0);
    positionedEyes.store(0);
    auto device = *reinterpret_cast<void**>(static_cast<char*>(application) + 0x1c0);
    const auto base = reinterpret_cast<char*>(GetModuleHandleW(nullptr));
#ifdef FS25VR_DESKTOP_CAPTURE
    if (device && connect_swapchain(device)) vr_desktop_capture(device, swapchain.Get(), record);
#endif
#ifndef FS25VR_DESKTOP_CAPTURE
    if ((vrEnabled || vr_session_ready()) && device && *static_cast<void**>(device) == base + 0xf85a00 &&
        vr_initialize(device, record) && connect_swapchain(device)) {
        using GetEngine = char*(*)();
        auto engine = reinterpret_cast<GetEngine>(base + 0x3fef30)();
        auto renderer = engine ? *reinterpret_cast<char**>(engine + 0xb8) : nullptr;
        auto camera = renderer ? *reinterpret_cast<void**>(renderer + 0x3e8) : nullptr;
        vr_begin_frame(swapchain.Get(), worldMode.load() && native_perspective(camera));
        if (!vrEnabled) vr_suppress_frame();
    }
#else
    (void)base;
#endif
    xrDrawing = vr_world_frame();
    publish_terrain_tracking();
    active.store(xrDrawing || (stereoEnabled.load() && !vr_frame_active()));
    originalRender(application);
    active.store(false);
    xrDrawing = false;
    if (frame <= 5 || ((stereoEnabled.load() || vr_frame_active()) && frame % 120 == 0)) {
        log("frame=%llu update_before=%llu update_after=%llu eye_outputs=%u eye_offsets=%u stereo=%u. "
            "Native stereo counters; headset acceptance unverified.", frame, before, updates.load(),
            completedEyes.load(), positionedEyes.load(), stereoEnabled.load() ? 1u : 0u);
    }
}

struct Patch {
    void** slot;
    void* expected;
    void* replacement;
};
}

void start_engine_probe(void (*logger)(const char*)) {
    record = logger;
    if (!approved_game_file()) {
        log("Native render probe disabled: host is not the exact approved FS25 executable.");
        return;
    }
    auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto pe = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE ||
        pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        pe->OptionalHeader.SizeOfImage != 36589568) {
        log("Native render probe disabled: loaded image identity mismatch.");
        return;
    }
    auto app = reinterpret_cast<void**>(base + 0xf01f40);
    auto view = reinterpret_cast<void**>(base + 0xf7d0f0);
    originalRender = reinterpret_cast<Render>(base + 0xbec20);
    originalUpdate = reinterpret_cast<Update>(base + 0xbe060);
    originalBind = reinterpret_cast<BindView>(base + 0x16c070);
    originalAfterEye = reinterpret_cast<AfterEye>(base + 0x16c100);
    auto counterSlot = find_import_slot(base, pe->OptionalHeader.SizeOfImage, "QueryPerformanceCounter");
    originalCounter = reinterpret_cast<Counter>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "QueryPerformanceCounter"));
    if (!counterSlot || !originalCounter || *counterSlot != reinterpret_cast<void*>(originalCounter)) {
        log("MR_ScarfaceVR adapter disabled: native timing import could not be verified for safe Lua registration.");
        return;
    }
    const std::array<Patch, 11> patches = {{
        {counterSlot, reinterpret_cast<void*>(originalCounter), reinterpret_cast<void*>(&counter)},
        {&app[7], reinterpret_cast<void*>(originalRender), reinterpret_cast<void*>(&render)},
        {&app[6], reinterpret_cast<void*>(originalUpdate), reinterpret_cast<void*>(&update)},
        {&view[7], base + 0x144730, reinterpret_cast<void*>(&view_count)},
        {&view[16], base + 0x1effd0, reinterpret_cast<void*>(&eye_offset)},
        {&view[14], base + 0x1eff80, reinterpret_cast<void*>(&head_translation)},
        {&view[15], base + 0x1eff90, reinterpret_cast<void*>(&head_rotation)},
        {&view[11], base + 0xa2120, reinterpret_cast<void*>(&projection)},
        {&view[18], base + 0x16bf40, reinterpret_cast<void*>(&view_width)},
        {&view[21], reinterpret_cast<void*>(originalBind), reinterpret_cast<void*>(&bind_view)},
        {&view[22], reinterpret_cast<void*>(originalAfterEye), reinterpret_cast<void*>(&after_eye)}
    }};
    for (const auto& patch : patches) {
        if (*patch.slot != patch.expected) {
            log("Native render probe disabled: an expected vtable target differs (RVA %llx).",
                static_cast<unsigned long long>(reinterpret_cast<unsigned char*>(patch.slot) - base));
            return;
        }
    }
    size_t applied = 0;
    for (const auto& patch : patches) {
        if (!replace_slot(patch.slot, patch.expected, patch.replacement)) {
            while (applied) {
                const auto& previous = patches[--applied];
                replace_slot(previous.slot, previous.replacement, previous.expected);
            }
            log("Native render probe disabled: vtable patch failed, earlier patches reverted.");
            return;
        }
        ++applied;
    }
    originalTerrainCamera = reinterpret_cast<GetTerrainCamera>(base + engine_address::terrainCameraInfo);
    if (redirect_relative_call(base + engine_address::terrainCameraCall,
            reinterpret_cast<void*>(originalTerrainCamera), reinterpret_cast<void*>(&terrain_camera))) {
        log("Terrain virtual-texture camera call verified and redirected; VR pose affects terrain detail selection only.");
    } else {
        log("Terrain virtual-texture correction unavailable: call guard/near relay failed; original terrain behavior retained.");
    }
    log("Approved FS25 SHA-256 verified. MR_ScarfaceVR OpenXR adapter installed. "
        "F9 desktop stereo diagnostic (default off), F10 recenter, F11 VR toggle (default off). Headset acceptance pending.");
}
