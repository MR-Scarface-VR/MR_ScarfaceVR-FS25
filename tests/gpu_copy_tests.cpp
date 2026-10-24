#include "../native/d3d12_copy.h"
#include <windows.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>

using Microsoft::WRL::ComPtr;
void ok(HRESULT result) { assert(SUCCEEDED(result)); }

int main() {
    ComPtr<IDXGIFactory4> factory;
    ok(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<ID3D12Device> device;
    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 description{};
        ok(adapter->GetDesc1(&description));
        if (!(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
            SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) break;
    }
    assert(device);
    D3D12_COMMAND_QUEUE_DESC queueInfo{};
    ComPtr<ID3D12CommandQueue> queue;
    ok(device->CreateCommandQueue(&queueInfo, IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;
    ok(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;
    ok(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
    D3D12_RESOURCE_DESC texture{};
    texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width = 64; texture.Height = 32;
    texture.DepthOrArraySize = texture.MipLevels = 1;
    texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.SampleDesc.Count = 1;
    texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> source, destination;
    ok(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texture,
        D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&source)));
    texture.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    ok(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texture,
        D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&destination)));
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 bytes = 0;
    device->GetCopyableFootprints(&texture, 0, 1, 0, &footprint, nullptr, nullptr, &bytes);
    footprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = bytes; buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> upload, readback;
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    ok(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)));
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    ok(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
    unsigned char* pixels = nullptr;
    D3D12_RANGE empty{0,0};
    ok(upload->Map(0, &empty, reinterpret_cast<void**>(&pixels)));
    for (UINT y = 0; y < 32; ++y) for (UINT x = 0; x < 64; ++x) {
        auto p = pixels + y*footprint.Footprint.RowPitch + x*4;
        p[0] = static_cast<unsigned char>(x*3); p[1] = static_cast<unsigned char>(y*7);
        p[2] = static_cast<unsigned char>(x^y); p[3] = 255;
    }
    upload->Unmap(0, nullptr);
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition = {source.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST};
    list->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION from{}, to{};
    from.pResource = upload.Get(); from.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    from.PlacedFootprint = footprint;
    to.pResource = source.Get(); to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    list->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    list->ResourceBarrier(1, &barrier);
    assert(copy_eye_image(list.Get(), source.Get(), destination.Get()));
    barrier.Transition = {destination.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE};
    list->ResourceBarrier(1, &barrier);
    from.pResource = destination.Get(); from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    to.pResource = readback.Get(); to.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    to.PlacedFootprint = footprint;
    list->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    list->ResourceBarrier(1, &barrier);
    ok(list->Close());
    ID3D12CommandList* submitted[] = {list.Get()};
    queue->ExecuteCommandLists(1, submitted);
    ComPtr<ID3D12Fence> fence;
    ok(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    assert(event);
    ok(queue->Signal(fence.Get(), 1));
    ok(fence->SetEventOnCompletion(1, event));
    if (WaitForSingleObject(event, 5000) != WAIT_OBJECT_0) ExitProcess(5);
    CloseHandle(event);
    D3D12_RANGE range{0, static_cast<SIZE_T>(bytes)};
    ok(readback->Map(0, &range, reinterpret_cast<void**>(&pixels)));
    for (UINT y = 0; y < 32; ++y) for (UINT x = 0; x < 64; ++x) {
        const auto p = pixels + y*footprint.Footprint.RowPitch + x*4;
        assert(p[0] == x*3 && p[1] == y*7 && p[2] == (x^y) && p[3] == 255);
    }
    readback->Unmap(0, &empty);
    std::puts("PASS: real hardware GPU copy UNORM -> OpenXR-style TYPELESS -> CPU, every pixel exact; no XR session opened");
}
