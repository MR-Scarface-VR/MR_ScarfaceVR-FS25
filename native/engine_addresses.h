#pragma once
#include <cstdint>

namespace engine_address {
inline constexpr uintptr_t capturedBase = 0x7ff7298d0000ull;
inline constexpr uintptr_t scriptGetter = 0x7ff729a04840ull - capturedBase;
inline constexpr uintptr_t scriptGlobal = 0x7ff72ba89ba0ull - capturedBase;
inline constexpr uintptr_t registerFunction = 0x7ff7299f7060ull - capturedBase;
inline constexpr uintptr_t terrainCameraCall = 0x7ff729d13dabull - capturedBase;
inline constexpr uintptr_t terrainCameraInfo = 0x7ff729d5d6c0ull - capturedBase;
}
