module;

#include <d3d12.h>
export module DirectXUtils;

namespace GiiGa
{
    export struct DirectXDeleter {
        void operator()(IUnknown* surface) {
            if (surface) surface->Release();
        }
    };
}
