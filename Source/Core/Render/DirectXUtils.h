#pragma once

#include<exception>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>

namespace GiiGa
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }

    // Deleter for COM objects held in std::shared_ptr.
    struct DXDeleter
    {
        template <typename T>
        void operator()(T* t)
        {
            if (t) t->Release();
        }
    };
}
