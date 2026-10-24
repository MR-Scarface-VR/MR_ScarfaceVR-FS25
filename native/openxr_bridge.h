#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12
#define XR_NO_PROTOTYPES
#include "../third_party/openxr/include/openxr/openxr.h"
#include "../third_party/openxr/include/openxr/openxr_platform.h"
#include "stereo.h"

// All calls belong to the game's render thread, outside the Windows loader lock.
bool vr_initialize(void* nativeDevice, void (*logger)(const char*));
bool vr_session_ready();
bool vr_runtime_loaded();
bool vr_begin_frame(IDXGISwapChain3* swapchain, bool world);
bool vr_frame_active();
bool vr_world_frame();
void vr_suppress_frame();
Pose vr_head_pose();
Vector3 vr_eye_offset(unsigned eye);
XrFovf vr_eye_fov(unsigned eye);
bool vr_copy_eye(void* nativeDevice, unsigned eye);
void vr_finish_frame(); // Only after the game has submitted its command lists.
void vr_recenter();
void vr_allow_output(bool enabled);
void vr_fail_closed(const char* reason);
void vr_request_stop();
void vr_capture_tick(bool submitted);
bool vr_desktop_capture(void* nativeDevice, IDXGISwapChain3* swapchain, void (*logger)(const char*));
void vr_shutdown();
