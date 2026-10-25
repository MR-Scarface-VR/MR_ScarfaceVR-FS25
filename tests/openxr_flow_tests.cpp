// Exercise the real submission code with a tiny runtime double; no headset required.
#include "../native/openxr_bridge.cpp"
#include <cassert>

namespace {
unsigned loaderCreates = 0;
bool expectReShadeLayer = false;
bool advertiseReShade = true, advertiseD3D12 = true;
XrResult createResult = XR_SUCCESS;
XrResult XRAPI_CALL loader_extensions(const char*, uint32_t capacity, uint32_t* count, XrExtensionProperties* out) {
    *count = advertiseD3D12 ? 1u : 0u;
    if (capacity) strcpy_s(out[0].extensionName, XR_KHR_D3D12_ENABLE_EXTENSION_NAME);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL loader_layers(uint32_t capacity, uint32_t* count, XrApiLayerProperties* out) {
    *count = advertiseReShade ? 1u : 0u;
    if (capacity) strcpy_s(out[0].layerName, "XR_APILAYER_reshade");
    return XR_SUCCESS;
}
XrResult XRAPI_CALL loader_create(const XrInstanceCreateInfo* info, XrInstance* out) {
    ++loaderCreates;
    assert(info->applicationInfo.apiVersion == XR_MAKE_VERSION(1, 0, 34));
    assert(info->enabledExtensionCount == 1);
    assert(info->enabledApiLayerCount == (expectReShadeLayer ? 1u : 0u));
    if (expectReShadeLayer) assert(strcmp(info->enabledApiLayerNames[0], "XR_APILAYER_reshade") == 0);
    if (XR_SUCCEEDED(createResult)) *out = reinterpret_cast<XrInstance>(static_cast<uintptr_t>(42));
    return createResult;
}
XrResult XRAPI_CALL loader_noop() { return XR_SUCCESS; }
XrResult XRAPI_CALL loader_get(XrInstance current, const char* name, PFN_xrVoidFunction* out) {
    if (strcmp(name, "xrEnumerateInstanceExtensionProperties") == 0) {
        assert(current == XR_NULL_HANDLE); *out = reinterpret_cast<PFN_xrVoidFunction>(loader_extensions);
    } else if (strcmp(name, "xrEnumerateApiLayerProperties") == 0) {
        assert(current == XR_NULL_HANDLE); *out = reinterpret_cast<PFN_xrVoidFunction>(loader_layers);
    } else if (strcmp(name, "xrCreateInstance") == 0) {
        assert(current == XR_NULL_HANDLE); *out = reinterpret_cast<PFN_xrVoidFunction>(loader_create);
    } else {
        assert(current != XR_NULL_HANDLE); *out = reinterpret_cast<PFN_xrVoidFunction>(loader_noop);
    }
    return XR_SUCCESS;
}
unsigned releases = 0, waits = 0, ends = 0;
uint32_t submittedLayers = 0;
XrStructureType submittedType = XR_TYPE_UNKNOWN;
XrResult waitResult = XR_SUCCESS;
XrTime submittedTime = 0;
XrResult XRAPI_CALL fake_wait(XrSwapchain, const XrSwapchainImageWaitInfo*) { ++waits; return waitResult; }
XrResult XRAPI_CALL fake_release(XrSwapchain, const XrSwapchainImageReleaseInfo*) { ++releases; return XR_SUCCESS; }
XrResult XRAPI_CALL fake_end(XrSession, const XrFrameEndInfo* info) {
    ++ends;
    submittedTime = info->displayTime;
    submittedLayers = info->layerCount;
    submittedType = info->layerCount ? info->layers[0]->type : XR_TYPE_UNKNOWN;
    if (submittedType == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
        const auto* layer = reinterpret_cast<const XrCompositionLayerProjection*>(info->layers[0]);
        assert(layer->viewCount == 2);
        assert(layer->views[0].subImage.swapchain == eyes[0].chain);
        assert(layer->views[1].subImage.swapchain == eyes[1].chain);
    }
    return XR_SUCCESS;
}
void setup(bool world, bool leftCopied, bool rightCopied) {
    apiWaitSwapchainImage = fake_wait;
    apiReleaseSwapchainImage = fake_release;
    apiEndFrame = fake_end;
    frameBegun = frameRenderable = true;
    allowOutput = true;
    worldFrame = world;
    displayTime = 987654321;
    imageWidth = 1920; imageHeight = 1080;
    eyes[0] = {}; eyes[1] = {};
    for (unsigned i = 0; i < 2; ++i) {
        eyes[i].chain = reinterpret_cast<XrSwapchain>(static_cast<uintptr_t>(i + 1));
        eyes[i].acquired = eyes[i].waited = true;
    }
    eyes[0].copied = leftCopied; eyes[1].copied = rightCopied;
    releases = waits = ends = 0;
    waitResult = XR_SUCCESS;
}
}
int main() {
    getProc = loader_get;
    assert(create_loader_instance(false) && loaderCreates == 1);
    instance = XR_NULL_HANDLE;
    expectReShadeLayer = true;
    assert(create_loader_instance(true) && loaderCreates == 2);
    instance = XR_NULL_HANDLE;
    createResult = XR_ERROR_INITIALIZATION_FAILED;
    assert(!create_loader_instance(true) && instance == XR_NULL_HANDLE);
    createResult = XR_SUCCESS;
    advertiseReShade = false;
    assert(!create_loader_instance(true) && loaderCreates == 3 && instance == XR_NULL_HANDLE);
    advertiseReShade = true;
    advertiseD3D12 = false;
    assert(!create_loader_instance(false) && loaderCreates == 3 && instance == XR_NULL_HANDLE);
    advertiseD3D12 = true;
    SetEnvironmentVariableW(L"XR_API_LAYER_PATH", L"existing-layer-directory");
    {
        ScopedApiLayerPath path(L"C:\\OwnVR test\\layers");
        assert(path.ready);
        wchar_t value[512];
        GetEnvironmentVariableW(L"XR_API_LAYER_PATH", value, 512);
        assert(wcscmp(value, L"existing-layer-directory;C:\\OwnVR test\\layers") == 0);
    }
    wchar_t restored[512];
    GetEnvironmentVariableW(L"XR_API_LAYER_PATH", restored, 512);
    assert(wcscmp(restored, L"existing-layer-directory") == 0);
    SetEnvironmentVariableW(L"XR_API_LAYER_PATH", nullptr);
    { ScopedApiLayerPath path(L"C:\\OwnVR test\\layers"); assert(path.ready); }
    assert(GetEnvironmentVariableW(L"XR_API_LAYER_PATH", restored, 512) == 0);
    getProc = nullptr;
#define RESET_XR(name) api##name = nullptr;
    XR_FUNCTIONS(RESET_XR)
#undef RESET_XR
    setup(true, true, true);
    vr_finish_frame();
    assert(releases == 2 && ends == 1 && submittedLayers == 1);
    assert(submittedType == XR_TYPE_COMPOSITION_LAYER_PROJECTION && submittedTime == 987654321);
    assert(!frameBegun && !frameRenderable);
    vr_finish_frame();
    assert(ends == 1);
    setup(true, true, false);
    vr_finish_frame();
    assert(submittedLayers == 0 && releases == 2);
    setup(false, true, false);
    vr_finish_frame();
    assert(submittedLayers == 1 && submittedType == XR_TYPE_COMPOSITION_LAYER_QUAD);
    setup(true, false, false);
    eyes[0].waited = false;
    waitResult = XR_TIMEOUT_EXPIRED;
    vr_finish_frame();
    assert(waits == 1 && releases == 1 && submittedLayers == 0);
    assert(eyes[0].acquired && !eyes[0].waited);
    assert(!drain_pending_images());
    waitResult = XR_SUCCESS;
    assert(drain_pending_images());
    assert(!eyes[0].acquired && releases == 2);
    setup(true, true, true);
    session = reinterpret_cast<XrSession>(static_cast<uintptr_t>(1));
    assert(!vr_begin_frame(nullptr, true));
    assert(!vr_frame_active());
    assert(eyes[0].acquired && eyes[1].acquired && releases == 0);
    vr_finish_frame();
    assert(submittedLayers == 0);
    session = XR_NULL_HANDLE;
    permanentlyDisabled = false;
    setup(true, false, false);
    eyes[0].waited = false;
    waitResult = XR_SESSION_LOSS_PENDING;
    vr_finish_frame();
    assert(releases == 2 && waits == 1 && !eyes[0].acquired && submittedLayers == 0);
    gpuPending = true;
    assert(!destroy_images());
    gpuPending = false;
    assert(gpu_work_retired(false, 7, 7));
    assert(!gpu_work_retired(false, 7, 6));
    assert(!gpu_work_retired(true, 7, 7));
    assert(!gpu_work_retired(false, 7, UINT64_MAX));
    permanentlyDisabled = false;
    setup(true, true, true);
    session = reinterpret_cast<XrSession>(static_cast<uintptr_t>(1));
    vr_request_stop();
    assert(!vr_begin_frame(nullptr, true) && releases == 0 && eyes[0].acquired);
    vr_finish_frame();
    assert(submittedLayers == 0 && releases == 2);
    assert(!vr_begin_frame(nullptr, true));
    assert(!session && !stopRequested);
    std::puts("PASS: loader dispatch/optional ReShade/failure paths/environment restore; XR stereo/quad submission, incomplete-eye rejection, ownership and frame pairing");
}
