module;

#include <memory>
#include <d3d12.h>

export module IRenderContext;


export import UploadBuffer;

namespace GiiGa
{
    export struct IRenderContext
    {
        virtual ~IRenderContext() = default;
        virtual UploadBuffer::Allocation CreateAndAllocateUploadBuffer(size_t size) =0;
        virtual void CopyBufferRegion(const std::shared_ptr<ID3D12Resource>& pDstBuffer, UINT64 DstOffset,
            const std::shared_ptr<ID3D12Resource>& pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) = 0;
        virtual void ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER& pBarriers) =0;
    };
}