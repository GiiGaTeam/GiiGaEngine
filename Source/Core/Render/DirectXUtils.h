#pragma once

#include<exception>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>

// NOTE: This file used to also contain std::hash / operator== specializations for all
// D3D12 view descriptors (to key the descriptor caches in GPULocalResource) and a
// frames-in-flight delayed COM deleter. Both are obsolete under NVRHI (it owns
// descriptors and resource lifetime), so only the generic COM helpers remain.

namespace GiiGa
{
    // Simple COM releaser for the few native D3D12/DXGI objects we still own directly
    // (the D3D12 device, command queue and DXGI swap chain).
    struct DXDeleter
    {
        void operator()(IUnknown* surface)
        {
            if (surface) surface->Release();
        }
    };

    inline void ThrowIfFailed(HRESULT hr)
    {
#ifndef NDEBUG
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception();
        }
#endif
    }
}
