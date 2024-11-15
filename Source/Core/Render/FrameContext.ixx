module;

#include<memory>
#include<vector>
#include<d3d12.h>

export module FrameContext;

import RenderDevice;
import DescriptorHeap;
import UploadBuffer;

namespace GiiGa
{
    export class FrameContext
    {
    public:
        FrameContext(RenderDevice& device):
            command_allocator(device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT)),
            common_allocator_(std::move(device.CreateDynamicSubAllocationManager(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))),
            sampler_allocator_(std::move(device.CreateDynamicSubAllocationManager(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)))
        {
            upload_buffers.reserve(4);
            upload_buffers.emplace_back(UploadBuffer(device));
        }

        void Reset()
        {
            command_allocator.reset();

            for (int i = 0; i < upload_buffers.size() - 1; ++i)
            {
                upload_buffers.pop_back();
            }

            upload_buffers[0].Reset();
        }

        UploadBuffer& CreateUploadBuffer(RenderDevice& device, size_t size)
        {
            upload_buffers.push_back(UploadBuffer(device, size));
            return *upload_buffers.end()--;
        }


        std::shared_ptr<ID3D12CommandAllocator> command_allocator;
        UINT64 FenceValue = 0;

    private:
        DynamicSuballocationsManager common_allocator_;
        DynamicSuballocationsManager sampler_allocator_;
        std::vector<UploadBuffer> upload_buffers;

    };
}