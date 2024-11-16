module;

#include<memory>
#include<span>
#include<algorithm>
#include<directx/d3dx12.h>

export module GPULocalBuffer;

import IRenderDevice;
import RenderContext;
import UploadBuffer;

namespace GiiGa
{
    export class GPULocalBuffer
    {
    public:
        GPULocalBuffer(IRenderDevice& device, size_t bufferSize, bool allowUA = false)
        {
            CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_RESOURCE_FLAGS flags = allowUA ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

            current_state_ = D3D12_RESOURCE_STATE_COPY_DEST;

            buffer_ = device.CreateCommittedResource(heapProperties, D3D12_HEAP_FLAG_NONE,
                resourceDesc, current_state_);
        }

        void UpdateContents(RenderContext& render_context, std::span<uint8_t> data, D3D12_RESOURCE_STATES stateAfer)
        {
            UploadBuffer::Allocation allocation = render_context.CreateAndAllocateUploadBuffer(data.size());

            std::copy(data.begin(), data.end(), allocation.CPU.begin());

            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                buffer_.get(),
                current_state_,
                D3D12_RESOURCE_STATE_COPY_DEST
                );

            render_context.ResourceBarrier(1, barrier);

            render_context.CopyBufferRegion(buffer_.get(), 0, allocation.resource.get(), 0, data.size());

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                buffer_.get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                stateAfer
                );

            render_context.ResourceBarrier(1, barrier);
        }

        ID3D12Resource* getRawResource()
        {
            return buffer_.get();
        }

    private:
        size_t size_;
        std::shared_ptr<ID3D12Resource> buffer_;
        D3D12_RESOURCE_STATES current_state_;
    };
}