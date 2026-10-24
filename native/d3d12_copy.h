#pragma once
#include <d3d12.h>

inline DXGI_FORMAT color_family(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    default: return format;
    }
}
inline bool copy_compatible(DXGI_FORMAT source, DXGI_FORMAT target) {
    // OpenXR returns TYPELESS resources even when creation requests a concrete format.
    return color_family(source) == color_family(target);
}

inline bool copy_eye_image(ID3D12GraphicsCommandList* list, ID3D12Resource* source, ID3D12Resource* target) {
    if (!list || !source || !target || source == target) return false;
    const auto a = source->GetDesc(), b = target->GetDesc();
    if (a.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || b.Dimension != a.Dimension ||
        a.Width != b.Width || a.Height != b.Height || a.DepthOrArraySize != 1 || b.DepthOrArraySize != 1 ||
        a.MipLevels != 1 || b.MipLevels != 1 || a.SampleDesc.Count != 1 || b.SampleDesc.Count != 1 ||
        !copy_compatible(a.Format, b.Format)) return false;
    D3D12_RESOURCE_BARRIER barriers[2]{};
    for (auto& barrier : barriers) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    barriers[0].Transition.pResource = source;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[1].Transition.pResource = target;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(2, barriers);
    D3D12_TEXTURE_COPY_LOCATION from{}, to{};
    from.pResource = source;
    to.pResource = target;
    from.Type = to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    list->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
    for (auto& barrier : barriers) {
        const auto before = barrier.Transition.StateBefore;
        barrier.Transition.StateBefore = barrier.Transition.StateAfter;
        barrier.Transition.StateAfter = before;
    }
    list->ResourceBarrier(2, barriers);
    return true;
}
