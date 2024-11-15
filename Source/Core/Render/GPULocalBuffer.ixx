module;

#include<memory>
#include<d3d12.h>
#include<directx/d3dx12.h>

export module GPULocalBuffer;

import IRenderDevice;

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

        void UpdateContents(void* data, size_t dataSize)
        {
            /*
            cpuDataMapGuard * = RenderContext::CreateUploadBuffer(dataSize)

            memcpy(cpuDataMapGuard, data, dataSize)

            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                buffer.Get(),
                stateBefore,
                D3D12_RESOURCE_STATE_COPY_DEST
                );

            RenderContext::ResourceBarrier(1, &barrier)

            RenderContext::CopyBufferRegion(buffer.Get(), 0, uploadBuffer.Get(), 0, dataSize)

            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                buffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                stateAfter
                );

            RenderContext::ResourceBarrier(1, &barrier)
            */
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