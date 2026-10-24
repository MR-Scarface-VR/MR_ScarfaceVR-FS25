#include "openxr_bridge.h"
#include "d3d12_copy.h"
#include "../third_party/openxr/include/openxr/openxr_loader_negotiation.h"
#include <wrl/client.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
HMODULE runtime = nullptr;
XrInstance instance = XR_NULL_HANDLE;
XrSystemId systemId = XR_NULL_SYSTEM_ID;
XrSession session = XR_NULL_HANDLE;
XrSpace localSpace = XR_NULL_HANDLE;
XrSpace viewSpace = XR_NULL_HANDLE;
PFN_xrGetInstanceProcAddr getProc = nullptr;
void (*record)(const char*) = nullptr;
ULONGLONG nextAttempt = 0;
bool permanentlyDisabled = false;
XrResult lastError = XR_SUCCESS;
bool running = false, frameBegun = false, frameRenderable = false, worldFrame = false;
bool needsRecenter = true;
XrTime displayTime = 0;
XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
Pose origin{{0,0,0,1}, {0,0,0}}, head{{0,0,0,1}, {0,0,0}};
Pose centeredHead = head;
std::array<XrView, 2> views = {{{XR_TYPE_VIEW}, {XR_TYPE_VIEW}}};
ComPtr<IDXGISwapChain3> gameSwapchain;
UINT imageWidth = 0, imageHeight = 0;
DXGI_FORMAT imageFormat = DXGI_FORMAT_UNKNOWN;
bool allowOutput = false;
bool stopRequested = false;
ComPtr<ID3D12Device> gameDevice;
ComPtr<ID3D12CommandQueue> graphicsQueue;
ComPtr<ID3D12Fence> captureFence;
ComPtr<ID3D12Resource> captureBuffer;
ComPtr<ID3D12Resource> desktopCaptureTarget;
D3D12_PLACED_SUBRESOURCE_FOOTPRINT captureLayout{};
bool captureRecorded = false, captureSubmitted = false, captureSaved = false;
uint64_t captureBytes = 0;
bool gpuPending = false;
uint64_t lastSubmission = 0, captureCompletion = 0;
unsigned submittedFrames = 0;
struct Eye {
    XrSwapchain chain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageD3D12KHR> images;
    uint32_t index = 0;
    bool acquired = false, waited = false, copied = false;
};
std::array<Eye, 2> eyes;

#define XR_FUNCTIONS(X) \
    X(DestroyInstance) X(GetSystem) X(GetD3D12GraphicsRequirementsKHR) \
    X(CreateSession) X(DestroySession) X(CreateReferenceSpace) X(DestroySpace) \
    X(PollEvent) X(BeginSession) X(EndSession) X(WaitFrame) X(BeginFrame) X(EndFrame) \
    X(LocateViews) X(LocateSpace) X(EnumerateSwapchainFormats) X(CreateSwapchain) \
    X(DestroySwapchain) X(EnumerateSwapchainImages) X(AcquireSwapchainImage) \
    X(WaitSwapchainImage) X(ReleaseSwapchainImage) X(EnumerateViewConfigurationViews)
#define DECLARE_XR(name) PFN_xr##name api##name = nullptr;
XR_FUNCTIONS(DECLARE_XR)
#undef DECLARE_XR

bool check(XrResult result, const char* operation) {
    if (XR_SUCCEEDED(result)) return true;
    if (record && result != lastError) {
        char message[256];
        sprintf_s(message, "OpenXR %s returned %d; desktop mode retained.", operation, result);
        record(message);
    }
    lastError = result;
    return false;
}

bool wait_complete(XrResult result) {
    if (result == XR_SESSION_LOSS_PENDING) {
        frameRenderable = false;
        permanentlyDisabled = true;
        if (record) record("OpenXR session loss pending; waited image retained for release, output disabled until restart.");
    }
    return result == XR_SUCCESS || result == XR_SESSION_LOSS_PENDING;
}

void save_capture() {
    if (!captureSubmitted || captureSaved || captureFence->GetCompletedValue() < captureCompletion) return;
    void* mapped = nullptr;
    D3D12_RANGE read{0, static_cast<SIZE_T>(captureBytes)};
    if (FAILED(captureBuffer->Map(0, &read, &mapped))) return;
    const auto width = captureLayout.Footprint.Width;
    const auto height = captureLayout.Footprint.Height;
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
    for (UINT y = 0; y < height; ++y) {
        const auto* row = static_cast<const unsigned char*>(mapped) + captureLayout.Offset +
                          static_cast<size_t>(y) * captureLayout.Footprint.RowPitch;
        auto* target = pixels.data() + static_cast<size_t>(y) * width * 4;
        for (UINT x = 0; x < width; ++x) {
            target[x*4] = row[x*4+2]; target[x*4+1] = row[x*4+1];
            target[x*4+2] = row[x*4]; target[x*4+3] = 255;
        }
    }
    D3D12_RANGE written{0,0};
    captureBuffer->Unmap(0, &written);
    HMODULE self = nullptr;
    wchar_t path[32768];
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&save_capture), &self) || !GetModuleFileNameW(self, path, _countof(path))) return;
    auto* slash = std::wcsrchr(path, L'\\');
    if (!slash) return;
    if (wcscpy_s(slash + 1, _countof(path) - static_cast<size_t>(slash + 1 - path), L"fs25-mr_scarfacevr-eye.bmp") != 0) return;
    HANDLE file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    BITMAPFILEHEADER header{};
    BITMAPINFOHEADER info{};
    header.bfType = 0x4d42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
    info.biSize = sizeof(info);
    info.biWidth = static_cast<LONG>(width);
    info.biHeight = -static_cast<LONG>(height);
    info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
    DWORD writtenBytes;
    const bool success = WriteFile(file, &header, sizeof(header), &writtenBytes, nullptr) &&
        WriteFile(file, &info, sizeof(info), &writtenBytes, nullptr) &&
        WriteFile(file, pixels.data(), static_cast<DWORD>(pixels.size()), &writtenBytes, nullptr);
    CloseHandle(file);
    captureSaved = success;
    if (record) record(success ? "GPU eye readback saved to fs25-mr_scarfacevr-eye.bmp; headset layers remain blocked by default." : "GPU eye readback write failed.");
}

template<class T> bool function(const char* name, T& output) {
    return check(getProc(instance, name, reinterpret_cast<PFN_xrVoidFunction*>(&output)), name) && output;
}

bool open_runtime() {
    wchar_t manifest[32768];
    DWORD bytes = sizeof(manifest);
    const auto status = RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1",
        L"ActiveRuntime", RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, nullptr, manifest, &bytes);
    if (status != ERROR_SUCCESS || bytes < sizeof(wchar_t) || bytes > sizeof(manifest)) {
        if (record) record("OpenXR inactive: no 64-bit active runtime registered.");
        return false;
    }
    manifest[_countof(manifest)-1] = 0;
    std::wstring path = manifest;
    const auto slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos || path.size() < 3 || path[1] != L':' ||
        _wcsicmp(path.c_str() + slash + 1, L"oculus_openxr_64.json") != 0) {
        if (record) record("OpenXR inactive: this build requires the registered Meta Link runtime.");
        return false;
    }
    // shortcut: Meta's installed runtime is negotiated directly; add a loader for other runtimes.
    path.resize(slash + 1);
    path += L"LibOVRRTImpl64_1.dll";
    runtime = LoadLibraryExW(path.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!runtime) {
        if (record) record("OpenXR inactive: installed Meta runtime could not be loaded.");
        return false;
    }
    auto negotiate = reinterpret_cast<PFN_xrNegotiateLoaderRuntimeInterface>(
        GetProcAddress(runtime, "xrNegotiateLoaderRuntimeInterface"));
    XrNegotiateLoaderInfo loader{};
    loader.structType = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
    loader.structVersion = XR_LOADER_INFO_STRUCT_VERSION;
    loader.structSize = sizeof(loader);
    loader.minInterfaceVersion = loader.maxInterfaceVersion = 1;
    loader.minApiVersion = XR_MAKE_VERSION(1, 0, 0);
    // Negotiation admits newer 1.x runtimes; the instance still requests only API 1.0.34.
    loader.maxApiVersion = XR_MAKE_VERSION(1, 0x3ff, 0xfff);
    XrNegotiateRuntimeRequest request{};
    request.structType = XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST;
    request.structVersion = XR_RUNTIME_INFO_STRUCT_VERSION;
    request.structSize = sizeof(request);
    if (!negotiate || !check(negotiate(&loader, &request), "negotiate") ||
        request.runtimeInterfaceVersion != 1 || !request.getInstanceProcAddr ||
        request.runtimeApiVersion < loader.minApiVersion ||
        XR_VERSION_MAJOR(request.runtimeApiVersion) != 1) return false;
    getProc = request.getInstanceProcAddr;
    PFN_xrEnumerateInstanceExtensionProperties enumerate = nullptr;
    PFN_xrCreateInstance create = nullptr;
    if (!function("xrEnumerateInstanceExtensionProperties", enumerate) ||
        !function("xrCreateInstance", create)) return false;
    uint32_t count = 0;
    if (!check(enumerate(nullptr, 0, &count, nullptr), "enumerate extensions") || count > 4096) return false;
    std::vector<XrExtensionProperties> extensions(count, {XR_TYPE_EXTENSION_PROPERTIES});
    if (!check(enumerate(nullptr, count, &count, extensions.data()), "enumerate extensions")) return false;
    bool d3d12 = false;
    for (const auto& extension : extensions) {
        if (std::strcmp(extension.extensionName, XR_KHR_D3D12_ENABLE_EXTENSION_NAME) == 0) d3d12 = true;
    }
    if (!d3d12) return false;
    const char* extension = XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
    XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(info.applicationInfo.applicationName, "MR_ScarfaceVR for FS25");
    strcpy_s(info.applicationInfo.engineName, "MR_ScarfaceVR native view adapter");
    info.applicationInfo.applicationVersion = 1;
    info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 34);
    info.enabledExtensionCount = 1;
    info.enabledExtensionNames = &extension;
    if (!check(create(&info, &instance), "create instance")) return false;
#define LOAD_XR(name) if (!function("xr" #name, api##name)) return false;
    XR_FUNCTIONS(LOAD_XR)
#undef LOAD_XR
    if (record) record("MR_ScarfaceVR OpenXR instance created through the installed Meta runtime; D3D12 extension available.");
    return true;
}

Pose pose(XrPosef value) {
    return {{value.orientation.x, value.orientation.y, value.orientation.z, value.orientation.w},
            {value.position.x, value.position.y, value.position.z}};
}

bool gpu_work_retired(bool pending, uint64_t last, uint64_t completed) {
    return !pending && completed != UINT64_MAX && completed >= last;
}

bool destroy_images() {
    const auto completed = captureFence ? captureFence->GetCompletedValue() : 0;
    if (!gpu_work_retired(gpuPending, lastSubmission, completed)) return false;
    for (const auto& eye : eyes) if (eye.chain && eye.acquired) return false;
    for (auto& eye : eyes) {
        if (eye.chain && apiDestroySwapchain && !check(apiDestroySwapchain(eye.chain), "destroy eye swapchain")) return false;
        eye = {};
    }
    gameSwapchain.Reset();
    imageWidth = imageHeight = 0;
    return true;
}

bool drain_pending_images() {
    for (auto& eye : eyes) {
        if (!eye.acquired) continue;
        if (!eye.waited) {
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            wait.timeout = 0;
            if (!wait_complete(apiWaitSwapchainImage(eye.chain, &wait))) return false;
            eye.waited = true;
        }
        XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        if (!check(apiReleaseSwapchainImage(eye.chain, &release), "release pending eye")) return false;
        eye.acquired = eye.waited = eye.copied = false;
    }
    return true;
}

bool create_images(IDXGISwapChain3* swapchain) {
    ComPtr<ID3D12Resource> buffer;
    if (!swapchain || FAILED(swapchain->GetBuffer(swapchain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&buffer)))) return false;
    const auto description = buffer->GetDesc();
    if (!description.Width || description.Width > INT32_MAX || !description.Height ||
        description.Height > INT32_MAX || description.SampleDesc.Count != 1 ||
        description.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) return false;
    if (gameSwapchain.Get() == swapchain && imageWidth == description.Width &&
        imageHeight == description.Height && imageFormat == description.Format) return true;
    // Previous images have been released only after game submission before this resize.
    if (!destroy_images()) return false;
    uint32_t count = 0;
    if (!check(apiEnumerateSwapchainFormats(session, 0, &count, nullptr), "formats") || count > 4096) return false;
    std::vector<int64_t> formats(count);
    if (!check(apiEnumerateSwapchainFormats(session, count, &count, formats.data()), "formats")) return false;
    bool supported = false;
    for (auto format : formats) if (format == description.Format) supported = true;
    if (!supported) {
        if (record) record("OpenXR inactive: desktop color format is not supported for exact eye-image copies.");
        return false;
    }
    XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    info.format = description.Format;
    info.sampleCount = 1;
    info.width = static_cast<uint32_t>(description.Width);
    info.height = description.Height;
    info.faceCount = info.arraySize = info.mipCount = 1;
    for (auto& eye : eyes) {
        if (!check(apiCreateSwapchain(session, &info, &eye.chain), "create eye swapchain") ||
            !check(apiEnumerateSwapchainImages(eye.chain, 0, &count, nullptr), "eye images") || !count || count > 64) {
            destroy_images();
            return false;
        }
        eye.images.assign(count, {XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
        if (!check(apiEnumerateSwapchainImages(eye.chain, count, &count,
            reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.images.data())), "eye images")) {
            destroy_images();
            return false;
        }
    }
    imageWidth = info.width;
    imageHeight = info.height;
    imageFormat = description.Format;
    gameSwapchain = swapchain;
    if (record) {
        char message[256];
        sprintf_s(message, "MR_ScarfaceVR OpenXR eye swapchains: %ux%u each, desktop-matched format %d. Headset quality/calibration unverified.",
            imageWidth, imageHeight, imageFormat);
        record(message);
    }
    return true;
}

bool poll_events() {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    XrResult result;
    while ((result = apiPollEvent(instance, &event)) == XR_SUCCESS) {
        if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            permanentlyDisabled = true;
            running = false;
            return false;
        }
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            const auto& state = reinterpret_cast<const XrEventDataSessionStateChanged&>(event);
            if (state.session == session) {
                sessionState = state.state;
                if (record) {
                    char message[128];
                    sprintf_s(message, "OpenXR session state=%d.", sessionState);
                    record(message);
                }
                if (sessionState == XR_SESSION_STATE_READY && !running) {
                    XrSessionBeginInfo info{XR_TYPE_SESSION_BEGIN_INFO};
                    info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    running = check(apiBeginSession(session, &info), "begin session");
                    needsRecenter = true;
                } else if (sessionState == XR_SESSION_STATE_STOPPING && running) {
                    check(apiEndSession(session), "end session");
                    running = false;
                } else if (sessionState == XR_SESSION_STATE_LOSS_PENDING || sessionState == XR_SESSION_STATE_EXITING) {
                    running = false;
                    permanentlyDisabled = true;
                }
            }
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
    return result == XR_EVENT_UNAVAILABLE || check(result, "poll events");
}
}

bool vr_initialize(void* nativeDevice, void (*logger)(const char*)) {
    record = logger;
    if (session) return true;
    if (permanentlyDisabled || !nativeDevice || GetTickCount64() < nextAttempt) return false;
    nextAttempt = GetTickCount64() + 5000;
    if (!instance && !open_runtime()) {
        vr_shutdown();
        permanentlyDisabled = true;
        return false;
    }
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (!check(apiGetSystem(instance, &systemInfo, &systemId), "get headset system")) return false;
    auto device = *reinterpret_cast<ID3D12Device**>(static_cast<char*>(nativeDevice) + 0x2b0);
    auto queue = *reinterpret_cast<ID3D12CommandQueue**>(static_cast<char*>(nativeDevice) + 0x2b8);
    if (!device || !queue) return false;
    ComPtr<ID3D12Device> queueDevice;
    ComPtr<IUnknown> deviceIdentity, queueDeviceIdentity;
    XrGraphicsRequirementsD3D12KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
    if (!check(apiGetD3D12GraphicsRequirementsKHR(instance, systemId, &requirements), "graphics requirements")) return false;
    const LUID luid = device->GetAdapterLuid();
    const D3D_FEATURE_LEVEL level = requirements.minFeatureLevel;
    D3D12_FEATURE_DATA_FEATURE_LEVELS levels{1, &level, {}};
    if (FAILED(queue->GetDevice(IID_PPV_ARGS(&queueDevice))) ||
        FAILED(device->QueryInterface(IID_PPV_ARGS(&deviceIdentity))) ||
        FAILED(queueDevice.As(&queueDeviceIdentity)) || deviceIdentity.Get() != queueDeviceIdentity.Get() ||
        queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT ||
        std::memcmp(&luid, &requirements.adapterLuid, sizeof(luid)) != 0 ||
        FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels)))) {
        if (record) record("OpenXR inactive: game device/queue/adapter does not meet Meta requirements.");
        permanentlyDisabled = true;
        return false;
    }
    XrGraphicsBindingD3D12KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};
    binding.device = device;
    binding.queue = queue;
    XrSessionCreateInfo info{XR_TYPE_SESSION_CREATE_INFO};
    info.next = &binding;
    info.systemId = systemId;
    if (!check(apiCreateSession(instance, &info, &session), "create session")) return false;
    XrReferenceSpaceCreateInfo space{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    space.poseInReferenceSpace.orientation.w = 1;
    space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    if (!check(apiCreateReferenceSpace(session, &space, &localSpace), "create local space")) {
        vr_shutdown();
        return false;
    }
    space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    if (!check(apiCreateReferenceSpace(session, &space, &viewSpace), "create view space")) {
        vr_shutdown();
        return false;
    }
    lastError = XR_SUCCESS;
    gameDevice = device;
    graphicsQueue = queue;
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&captureFence)))) {
        vr_shutdown();
        return false;
    }
    if (record) record("MR_ScarfaceVR OpenXR session created on the game's verified D3D12 device and direct queue. No images submitted yet.");
    return true;
}

bool vr_session_ready() { return session != XR_NULL_HANDLE; }
bool vr_runtime_loaded() { return instance != XR_NULL_HANDLE; }

void vr_capture_tick(bool submitted) {
    if (submitted && gpuPending && graphicsQueue && captureFence) {
        if (SUCCEEDED(graphicsQueue->Signal(captureFence.Get(), ++lastSubmission))) {
            gpuPending = false;
            if (captureRecorded && !captureSubmitted) {
                captureSubmitted = true;
                captureCompletion = lastSubmission;
            }
        } else vr_fail_closed("OpenXR disabled: graphics queue fence signal failed; resources retained.");
    }
    save_capture();
}

bool vr_desktop_capture(void* nativeDevice, IDXGISwapChain3* swapchain, void (*logger)(const char*)) {
    record = logger;
    vr_capture_tick(false);
    if (captureRecorded) { frameRenderable = false; return false; }
    if (desktopCaptureTarget) return true;
    gameDevice = *reinterpret_cast<ID3D12Device**>(static_cast<char*>(nativeDevice) + 0x2b0);
    graphicsQueue = *reinterpret_cast<ID3D12CommandQueue**>(static_cast<char*>(nativeDevice) + 0x2b8);
    ComPtr<ID3D12Resource> source;
    if (!gameDevice || !graphicsQueue || FAILED(swapchain->GetBuffer(swapchain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&source)))) return false;
    auto description = source->GetDesc();
    if (description.Format != DXGI_FORMAT_R8G8B8A8_UNORM) return false;
    imageWidth = static_cast<UINT>(description.Width);
    imageHeight = description.Height;
    imageFormat = description.Format;
    description.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    if (FAILED(gameDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description,
        D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&desktopCaptureTarget))) ||
        FAILED(gameDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&captureFence)))) return false;
    gameSwapchain = swapchain;
    eyes[0].images.assign(1, {XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
    eyes[0].images[0].texture = desktopCaptureTarget.Get();
    eyes[0].waited = true;
    frameRenderable = true;
    worldFrame = false;
    if (record) record("Desktop-only native GPU capture armed with own TYPELESS image; no OpenXR instance/session is created.");
    return true;
}

bool vr_begin_frame(IDXGISwapChain3* swapchain, bool world) {
    save_capture();
    if (stopRequested) {
        frameRenderable = false;
        if (!frameBegun && drain_pending_images()) vr_shutdown();
        return false;
    }
    if (frameBegun) {
        frameRenderable = false;
        permanentlyDisabled = true;
        if (record) record("OpenXR disabled until restart: previous game frame reached no confirmed Present; pending images retained safely.");
        return false;
    }
    frameRenderable = false;
    if (!session || permanentlyDisabled || frameBegun || !poll_events() || !running) return false;
    if (!drain_pending_images()) return false;
    if (!create_images(swapchain)) return false;
    XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState state{XR_TYPE_FRAME_STATE};
    if (!check(apiWaitFrame(session, &wait, &state), "wait frame")) return false;
    XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};
    if (!check(apiBeginFrame(session, &begin), "begin frame")) return false;
    frameBegun = true;
    frameRenderable = false;
    displayTime = state.predictedDisplayTime;
    worldFrame = world;
    if (!state.shouldRender) return false;
    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
    constexpr XrSpaceLocationFlags locationFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
    constexpr XrSpaceLocationFlags trackingFlags = XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
    if (!check(apiLocateSpace(viewSpace, localSpace, displayTime, &location), "locate head") ||
        (location.locationFlags & (locationFlags | trackingFlags)) != (locationFlags | trackingFlags)) return false;
    head = pose(location.pose);
    if (!valid_pose(head)) return false;
    if (needsRecenter) { origin = recenter_origin(head); needsRecenter = false; }
    centeredHead = relative_pose(origin, head);
    XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO};
    locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locate.displayTime = displayTime;
    locate.space = localSpace;
    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t count = 0;
    if (!check(apiLocateViews(session, &locate, &viewState, 2, &count, views.data()), "locate eyes") || count != 2 ||
        (viewState.viewStateFlags & (XR_VIEW_STATE_ORIENTATION_VALID_BIT | XR_VIEW_STATE_POSITION_VALID_BIT)) !=
        (XR_VIEW_STATE_ORIENTATION_VALID_BIT | XR_VIEW_STATE_POSITION_VALID_BIT)) return false;
    for (unsigned i = 0; i < 2; ++i) {
        const auto eyePose = pose(views[i].pose);
        if (!valid_pose(eyePose)) return false;
        const auto relative = relative_pose(head, eyePose);
        // Quest 2 has parallel eye orientations; a canted headset needs per-eye native rotation.
        if (std::abs(relative.orientation.x) + std::abs(relative.orientation.y) +
            std::abs(relative.orientation.z) > 0.002f) return false;
        auto& eye = eyes[i];
        XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (!check(apiAcquireSwapchainImage(eye.chain, &acquire, &eye.index), "acquire eye")) return false;
        eye.acquired = true;
        if (eye.index >= eye.images.size()) { permanentlyDisabled = true; return false; }
        XrSwapchainImageWaitInfo imageWait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        imageWait.timeout = 100000000; // 100 ms, with ownership retained on timeout.
        const auto result = apiWaitSwapchainImage(eye.chain, &imageWait);
        if (!wait_complete(result)) {
            if (result != XR_TIMEOUT_EXPIRED) check(result, "wait eye");
            return false;
        }
        eye.waited = true;
        if (permanentlyDisabled) return false;
    }
    frameRenderable = true;
    return true;
}

bool vr_frame_active() { return frameRenderable; }
bool vr_world_frame() { return frameRenderable && worldFrame; }
void vr_suppress_frame() { frameRenderable = false; }
Pose vr_head_pose() { return centeredHead; }
Vector3 vr_eye_offset(unsigned eye) {
    return eye < 2 ? relative_pose(head, pose(views[eye].pose)).position : Vector3{};
}
XrFovf vr_eye_fov(unsigned eye) { return eye < 2 ? views[eye].fov : XrFovf{}; }
void vr_recenter() { needsRecenter = true; }
void vr_allow_output(bool enabled) { allowOutput = enabled; }
void vr_request_stop() { stopRequested = true; allowOutput = false; frameRenderable = false; }
void vr_fail_closed(const char* reason) {
    frameRenderable = false;
    permanentlyDisabled = true;
    if (record) record(reason);
}

bool vr_copy_eye(void* nativeDevice, unsigned index) {
    if (!frameRenderable || index >= 2 || (!worldFrame && index != 0)) return false;
    auto& eye = eyes[index];
    if (!eye.waited || eye.copied || !gameSwapchain) return false;
    auto context = *reinterpret_cast<char**>(static_cast<char*>(nativeDevice) + 0x2d8);
    auto list = context ? *reinterpret_cast<ID3D12GraphicsCommandList**>(context + 0xfb8) : nullptr;
    if (!list) return false;
    ComPtr<ID3D12Resource> source;
    if (FAILED(gameSwapchain->GetBuffer(gameSwapchain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&source)))) return false;
    ID3D12Resource* destination = eye.images[eye.index].texture;
    const auto sourceDescription = source->GetDesc();
    const auto targetDescription = destination->GetDesc();
    if (sourceDescription.Width != imageWidth || sourceDescription.Height != imageHeight ||
        sourceDescription.Format != imageFormat || targetDescription.Width != imageWidth ||
        targetDescription.Height != imageHeight) return false;
    if (!copy_eye_image(list, source.Get(), destination)) return false;
    D3D12_TEXTURE_COPY_LOCATION to{};
    to.pResource = destination;
    to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    if (index == 0 && !captureRecorded && imageFormat == DXGI_FORMAT_R8G8B8A8_UNORM && gameDevice) {
        if (!captureBuffer) {
            gameDevice->GetCopyableFootprints(&targetDescription, 0, 1, 0, &captureLayout, nullptr, nullptr, &captureBytes);
            D3D12_HEAP_PROPERTIES heap{};
            heap.Type = D3D12_HEAP_TYPE_READBACK;
            D3D12_RESOURCE_DESC description{};
            description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            description.Width = captureBytes;
            description.Height = description.DepthOrArraySize = description.MipLevels = 1;
            description.SampleDesc.Count = 1;
            description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            gameDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&captureBuffer));
        }
        if (captureBuffer) {
            D3D12_RESOURCE_BARRIER captureBarrier{};
            captureBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            captureBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            captureBarrier.Transition.pResource = destination;
            captureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            captureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            list->ResourceBarrier(1, &captureBarrier);
            D3D12_TEXTURE_COPY_LOCATION readback{};
            readback.pResource = captureBuffer.Get();
            readback.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            readback.PlacedFootprint = captureLayout;
            list->CopyTextureRegion(&readback, 0, 0, 0, &to, nullptr);
            std::swap(captureBarrier.Transition.StateBefore, captureBarrier.Transition.StateAfter);
            list->ResourceBarrier(1, &captureBarrier);
            captureRecorded = true;
        }
    }
    eye.copied = true;
    gpuPending = true;
    return true;
}

void vr_finish_frame() {
    if (!frameBegun) return;
    vr_capture_tick(true);
    bool complete = frameRenderable && eyes[0].copied && (!worldFrame || eyes[1].copied);
    for (auto& eye : eyes) {
        if (!eye.acquired) continue;
        if (!eye.waited) {
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            wait.timeout = 0;
            if (wait_complete(apiWaitSwapchainImage(eye.chain, &wait))) eye.waited = true;
        }
        if (eye.waited) {
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            if (check(apiReleaseSwapchainImage(eye.chain, &release), "release eye")) {
                eye.acquired = eye.waited = false;
            } else complete = false;
        } else complete = false;
        eye.copied = false;
    }
    complete = complete && frameRenderable && allowOutput;
    std::array<XrCompositionLayerProjectionView, 2> projectionViews = {{
        {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}}};
    XrCompositionLayerProjection projection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projection.space = localSpace;
    projection.viewCount = 2;
    projection.views = projectionViews.data();
    for (unsigned i = 0; i < 2; ++i) {
        projectionViews[i].pose = views[i].pose;
        projectionViews[i].fov = views[i].fov;
        projectionViews[i].subImage.swapchain = eyes[i].chain;
        projectionViews[i].subImage.imageRect.extent = {static_cast<int32_t>(imageWidth), static_cast<int32_t>(imageHeight)};
    }
    XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
    quad.space = viewSpace;
    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    quad.subImage = projectionViews[0].subImage;
    quad.pose.orientation.w = 1;
    quad.pose.position.z = -2;
    quad.size = {3.0f, 3.0f * static_cast<float>(imageHeight) / static_cast<float>(imageWidth)};
    const auto* layer = worldFrame ? reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection) :
                                    reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad);
    XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};
    end.displayTime = displayTime;
    end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    end.layerCount = complete ? 1u : 0u;
    end.layers = complete ? &layer : nullptr;
    const auto result = apiEndFrame(session, &end);
    check(result, "end frame");
    if (++submittedFrames <= 5 && record) {
        char message[256];
        sprintf_s(message, "OpenXR frame end: result=%d layers=%u output_allowed=%u capture_recorded=%u capture_submitted=%u.",
            result, end.layerCount, allowOutput ? 1u : 0u, captureRecorded ? 1u : 0u, captureSubmitted ? 1u : 0u);
        record(message);
    }
    frameBegun = frameRenderable = false;
}

void vr_shutdown() {
    // Caller must have completed game GPU submission and the current XR frame first.
    if (!destroy_images()) {
        vr_fail_closed("OpenXR teardown deferred: GPU work/image ownership not retired; resources retained until process exit.");
        return;
    }
    if (viewSpace && apiDestroySpace) apiDestroySpace(viewSpace);
    if (localSpace && apiDestroySpace) apiDestroySpace(localSpace);
    viewSpace = localSpace = XR_NULL_HANDLE;
    if (session && apiDestroySession) apiDestroySession(session);
    session = XR_NULL_HANDLE;
    if (instance && apiDestroyInstance) apiDestroyInstance(instance);
    instance = XR_NULL_HANDLE;
    systemId = XR_NULL_SYSTEM_ID;
    getProc = nullptr;
    if (runtime) FreeLibrary(runtime);
    runtime = nullptr;
    running = frameBegun = frameRenderable = false;
    stopRequested = false;
    needsRecenter = true;
    sessionState = XR_SESSION_STATE_UNKNOWN;
    captureBuffer.Reset();
    captureFence.Reset();
    graphicsQueue.Reset();
    gameDevice.Reset();
    captureRecorded = captureSubmitted = captureSaved = false;
    lastSubmission = captureCompletion = 0;
}
