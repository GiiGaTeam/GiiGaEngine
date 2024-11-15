module;

#include<exception>
#include <d3d12.h>

export module DirectXUtils;

namespace GiiGa
{
    export struct DirectXDeleter
    {
        void operator()(IUnknown* surface)
        {
            if (surface) surface->Release();
        }
    };

    export inline void ThrowIfFailed(HRESULT hr)
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