export module IRenderContext;

export import <memory>;
export import <vector>;
export import <span>;
import <d3d12.h>;
import <functional>;

export import UploadBuffer;

namespace GiiGa
{
    export struct DefferedUploadQuery
    {
        std::function<D3D12_RESOURCE_STATES()> getStateFunction;
        std::function<void (D3D12_RESOURCE_STATES)> setStateFunction;
        D3D12_RESOURCE_STATES stateAfter;
        std::shared_ptr<ID3D12Resource> destination;
    };
    
    export struct IRenderContext
    {
        virtual ~IRenderContext() = default;
        virtual void CreateDefferedUpload(const std::span<const uint8_t>& data, DefferedUploadQuery&&)=0;
        virtual UploadBuffer::Allocation CreateAndAllocateUploadBufferInCurrentFrame(size_t size) =0;
        virtual void CopyBufferRegion(const std::shared_ptr<ID3D12Resource>& pDstBuffer, UINT64 DstOffset,
            const std::shared_ptr<ID3D12Resource>& pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) = 0;
        virtual void ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER& pBarriers) =0;
    };
}