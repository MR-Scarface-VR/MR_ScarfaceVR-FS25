// Exercise the real submission code with a tiny runtime double; no headset required.
#include "../native/openxr_bridge.cpp"
#include <cassert>

namespace {
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
    std::puts("PASS: XR stereo/quad submission, incomplete-eye rejection, wait-timeout ownership and frame pairing");
}
