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
        GPULocalBuffer(IRenderDevice& device, size_t bufferSize, bool allowUA = false,
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST):
            size_(bufferSize)
        {
            CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_RESOURCE_FLAGS flags = allowUA ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size_, flags);

            current_state_ = initialState;

            buffer_ = device.CreateCommittedResource(heapProperties, D3D12_HEAP_FLAG_NONE,
                resourceDesc, current_state_);
        }

        void UpdateContents(RenderContext& render_context, std::span<uint8_t> data, D3D12_RESOURCE_STATES stateAfer)
        {
            UploadBuffer::Allocation allocation = render_context.CreateAndAllocateUploadBuffer(data.size());

            std::copy(data.begin(), data.end(), allocation.CPU.begin());

            StateTransition(render_context, D3D12_RESOURCE_STATE_COPY_DEST);

            render_context.CopyBufferRegion(buffer_.get(), 0, allocation.resource.get(), 0, data.size());

            StateTransition(render_context, stateAfer);
        }

        void StateTransition(RenderContext& render_context, D3D12_RESOURCE_STATES stateAfer)
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                buffer_.get(),
                current_state_,
                stateAfer
                );

            render_context.ResourceBarrier(1, barrier);

            current_state_ = stateAfer;
        }

        std::shared_ptr<ID3D12Resource> getRawResource()
        {
            return buffer_;
        }

    private:
        size_t size_;
        std::shared_ptr<ID3D12Resource> buffer_;
        D3D12_RESOURCE_STATES current_state_;
    };
}