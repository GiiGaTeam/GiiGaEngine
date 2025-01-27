﻿#pragma once

#include<memory>
#include<vector>
#include<span>

#include<RenderDevice.h>
#include<DescriptorHeap.h>
#include<UploadBuffer.h>
#include<BufferView.h>

namespace GiiGa
{
    class FrameContext
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

        FrameContext(const FrameContext& other) = delete;
        FrameContext(FrameContext&& other) noexcept = default;
        FrameContext& operator=(const FrameContext& other) = delete;
        FrameContext& operator=(FrameContext&& other) noexcept = delete;

        ~FrameContext()
        {
            common_allocator_.ReleaseAllocations(0);
            sampler_allocator_.ReleaseAllocations(0);
        }

        void Reset(std::shared_ptr<ID3D12GraphicsCommandList>& command_list)
        {
            command_allocator->Reset();
            command_list->Reset(command_allocator.get(), nullptr);

            common_allocator_.ReleaseAllocations(0);
            sampler_allocator_.ReleaseAllocations(0);

            for (int i = 0; i < upload_buffers.size() - 1; ++i)
            {
                upload_buffers.pop_back();
            }

            upload_buffers[0].Reset();
        }

        UploadBuffer& CreateUploadBuffer(RenderDevice& device, size_t size)
        {
            upload_buffers.push_back(UploadBuffer(device, size));
            return upload_buffers.back();
        }

        UploadBuffer::Allocation AllocateCopyDynamic(RenderDevice& device, std::span<uint8_t> data, size_t alignment)
        {
            UploadBuffer::Allocation res_alloc = upload_buffers[0].Allocate(data.size(), alignment);
            std::copy(data.begin(), data.end(), res_alloc.CPU.begin());

            return res_alloc;
        }

        std::shared_ptr<BufferView<Constant>> AllocateDynamicConstantView(RenderDevice& device, std::span<uint8_t> data, size_t alignment,
                                                                          D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
        {
            UploadBuffer::Allocation res_alloc = AllocateCopyDynamic(device, data, alignment);

            DescriptorHeapAllocation desc_alloc = common_allocator_.Allocate(1);
            desc.BufferLocation = res_alloc.GPU;
            return device.CreateConstantBufferView(desc, std::move(desc_alloc));
        }

        std::shared_ptr<BufferView<ShaderResource>> AllocateDynamicShaderResourceView(RenderDevice& device, std::span<uint8_t> data, size_t alignment,
                                                                                      D3D12_SHADER_RESOURCE_VIEW_DESC desc)
        {
            auto& upload_buf = CreateUploadBuffer(device, desc.Buffer.NumElements * desc.Buffer.StructureByteStride);
            UploadBuffer::Allocation res_alloc = upload_buf.Allocate(data.size(), alignment);
            std::copy(data.begin(), data.end(), res_alloc.CPU.begin());

            DescriptorHeapAllocation desc_alloc = common_allocator_.Allocate(1);
            return device.CreateShaderResourceBufferView(res_alloc.resource, desc, std::move(desc_alloc));
        }


        UINT64 FenceValue = 0;
        std::shared_ptr<ID3D12CommandAllocator> command_allocator;

    private:
        DynamicSuballocationsManager common_allocator_;
        DynamicSuballocationsManager sampler_allocator_;
        std::vector<UploadBuffer> upload_buffers;
    };
}
