#include "../native/engine_hooks.h"
#include "../native/stereo.h"
#include "../native/engine_addresses.h"
#include "../native/d3d12_copy.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <array>
#include <cstring>

namespace {
int first() { return 1; }
int second() { return 2; }
}

int main() {
    std::array<unsigned char, 4096> image{};
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
    dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 128;
    auto pe = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.data()+128);
    pe->Signature = IMAGE_NT_SIGNATURE;
    pe->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    pe->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] = {512, 40};
    auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image.data()+512);
    descriptor->Name = 800; descriptor->FirstThunk = 600; descriptor->OriginalFirstThunk = 648;
    auto names = reinterpret_cast<IMAGE_THUNK_DATA64*>(image.data()+648);
    names->u1.AddressOfData = 704;
    std::memcpy(image.data()+706, "QueryPerformanceCounter", 24);
    assert(find_import_slot(image.data(), image.size(), "QueryPerformanceCounter") == reinterpret_cast<void**>(image.data()+600));
    assert(find_import_slot(image.data(), image.size(), "NotImported") == nullptr);
    assert(find_import_slot(image.data(), 540, "QueryPerformanceCounter") == nullptr);
    std::array<char, 0xf0> script{};
    void* state = reinterpret_cast<void*>(1);
    std::memcpy(script.data()+0x10, &state, sizeof(state));
    script[0xe0] = 1;
    assert(!script_registration_ready(script.data()));
    script[0xe0] = 0;
    assert(script_registration_ready(script.data()));
    assert(copy_compatible(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_TYPELESS));
    assert(copy_compatible(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_TYPELESS));
    assert(!copy_compatible(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_TYPELESS));
    assert(!copy_compatible(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R16G16B16A16_TYPELESS));
    assert(engine_address::scriptGetter == 0x7ff729a04840ull - engine_address::capturedBase);
    std::array<char, 0x170> camera{};
    assert(native_perspective(camera.data()));
    camera[0x168] = 1;
    assert(!native_perspective(camera.data()));
    assert(!native_perspective(nullptr));
    alignas(void*) std::array<char, 0x390> backend{};
    alignas(void*) std::array<char, 0xc0> window{};
    void* backendTable = reinterpret_cast<void*>(1);
    void* windowTable = reinterpret_cast<void*>(2);
    auto windowPointer = window.data();
    void* chain = reinterpret_cast<void*>(3);
    std::memcpy(backend.data(), &backendTable, sizeof(void*));
    std::memcpy(backend.data()+0x388, &windowPointer, sizeof(void*));
    std::memcpy(window.data(), &windowTable, sizeof(void*));
    std::memcpy(window.data()+0xb0, &chain, sizeof(void*));
    assert(native_swapchain(backend.data(), backendTable, windowTable) == chain);
    chain = reinterpret_cast<void*>(4);
    std::memcpy(window.data()+0xb0, &chain, sizeof(void*));
    assert(native_swapchain(backend.data(), backendTable, windowTable) == chain);
    assert(native_swapchain(backend.data(), windowTable, windowTable) == nullptr);
    constexpr float s = 0.70710678118f;
    const Quaternion turn{0, s, 0, s};
    const auto rotated = rotate(turn, {0, 0, -1});
    assert(std::abs(rotated.x + 1) < 0.00001f && std::abs(rotated.z) < 0.00001f);
    const Pose head{turn, {3, 2, 5}};
    const auto center = recenter_origin(head);
    const auto centered = relative_pose(center, head);
    assert(std::abs(centered.position.x) < 0.00001f);
    assert(std::abs(centered.position.y) < 0.00001f);
    assert(std::abs(centered.position.z) < 0.00001f);
    assert(std::abs(centered.orientation.w - 1) < 0.00001f);
    const auto moved = relative_pose(center, Pose{turn, {2, 2.25f, 5}});
    assert(std::abs(moved.position.x) < 0.00001f);
    assert(std::abs(moved.position.y - 0.25f) < 0.00001f);
    assert(std::abs(moved.position.z + 1) < 0.00001f);
    const auto eyeWorld = rotate(turn, {-0.031f, 0, 0});
    const auto eyeLocal = rotate(conjugate(turn), eyeWorld);
    assert(std::abs(eyeLocal.x + 0.031f) < 0.00001f);
    float matrix[16];
    rotation_matrix(turn, matrix);
    assert(std::abs(matrix[8] - 1) < 0.00001f && std::abs(matrix[2] + 1) < 0.00001f);
    assert(matrix[15] == 1 && matrix[12] == 0);
    assert(valid_pose(head));
    assert(!valid_pose(Pose{{0, 0, 0, 0}, {0, 0, 0}}));
    assert(!valid_pose(Pose{{0, 0, 0, 1}, {std::numeric_limits<float>::infinity(), 0, 0}}));
    const auto left = diagnostic_eye_offset(0);
    const auto right = diagnostic_eye_offset(1);
    assert(std::abs(left.x + 0.032f) < 0.000001f);
    assert(std::abs(right.x - 0.032f) < 0.000001f);
    assert(left.y == 0 && left.z == 0 && right.y == 0 && right.z == 0);
    assert(diagnostic_eye_offset(2).x == 0);

    void* slot = reinterpret_cast<void*>(&first);
    assert(!replace_slot(&slot, reinterpret_cast<void*>(&second), reinterpret_cast<void*>(&first)));
    assert(slot == reinterpret_cast<void*>(&first));
    assert(replace_slot(&slot, reinterpret_cast<void*>(&first), reinterpret_cast<void*>(&second)));
    assert(slot == reinterpret_cast<void*>(&second));
    assert(reinterpret_cast<int(*)()>(slot)() == 2);
    assert(replace_slot(&slot, reinterpret_cast<void*>(&second), reinterpret_cast<void*>(&first)));
    assert(reinterpret_cast<int(*)()>(slot)() == 1);
    assert(!replace_slot(nullptr, reinterpret_cast<void*>(&first), reinterpret_cast<void*>(&second)));
    std::puts("PASS: eye offsets and guarded real vtable-slot replacement");
}
