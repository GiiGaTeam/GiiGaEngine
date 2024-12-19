module;

#include <memory>
#include <d3d12.h>

export module IRenderContext;

export import <span>;

export import UploadBuffer;

namespace GiiGa
{
    /* potential problems:
    * - span and current_state can outlive the source, if owner gets deleted while deffered wait in queue
    *   So UB scenario
    * - why reference to current_state?
    *   - if not we get, not really clear state transitioning
    *   - where to set current state?
    *      * for example we can make deffered update in middle of frame (From State A [current] -> B)
    *       after in same frame make immediate update (B [current]->C) but object actually is in state A
    *       this will mess it up
    *  may be we can make it with std::function callback which capture object by shared_ptr
    */
    export struct DefferedUploadEntry
    {
        const std::span<const uint8_t>& data;
        D3D12_RESOURCE_STATES& current_state;
        D3D12_RESOURCE_STATES stateAfter;
        std::shared_ptr<ID3D12Resource> destination;
    };
    
    export struct IRenderContext
    {
        virtual ~IRenderContext() = default;
        virtual void CreateDefferedUpload(DefferedUploadEntry)=0;
        virtual UploadBuffer::Allocation CreateAndAllocateUploadBuffer(size_t size) =0;
        virtual void CopyBufferRegion(const std::shared_ptr<ID3D12Resource>& pDstBuffer, UINT64 DstOffset,
            const std::shared_ptr<ID3D12Resource>& pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) = 0;
        virtual void ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER& pBarriers) =0;
    };
}