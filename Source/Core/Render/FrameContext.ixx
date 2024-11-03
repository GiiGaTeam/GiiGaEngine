module;

#include<memory>
#include<d3d12.h>

export module FrameContext;

namespace GiiGa
{
    export struct FrameContext
    {
        std::shared_ptr<ID3D12CommandAllocator> command_allocator;
        UINT64 FenceValue=0;
    };
}