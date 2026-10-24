#include "../native/openxr_bridge.h"
#include <wrl/client.h>
#include <array>
#include <cstdio>
#include <cstring>

int main() {
    using Microsoft::WRL::ComPtr;
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return 1;
    ComPtr<ID3D12Device> device;
    for (UINT index = 0; ; ++index) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 description{};
        adapter->GetDesc1(&description);
        if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) break;
    }
    if (!device) return 2;
    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC description{};
    description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(&description, IID_PPV_ARGS(&queue)))) return 3;
    alignas(void*) std::array<char, 0x300> native{};
    const auto devicePointer = device.Get();
    const auto queuePointer = queue.Get();
    std::memcpy(native.data() + 0x2b0, &devicePointer, sizeof(devicePointer));
    std::memcpy(native.data() + 0x2b8, &queuePointer, sizeof(queuePointer));
    const bool ready = vr_initialize(native.data(), [](const char* message) { std::puts(message); });
    const bool loaded = vr_runtime_loaded();
    std::printf("Runtime probe: headset session=%s (no frames submitted).\n", ready ? "created" : "inactive");
    vr_shutdown();
    return loaded ? 0 : 4;
}
