module;

#include<memory>
#include<vector>
#include<d3d12.h>

export module RenderContext;

import RenderSystemSettings;
import RenderDevice;
import FrameContext;
import Window;
import UploadBuffer;

namespace GiiGa
{

    export class RenderContext final
    {
    public:
        void Create(RenderDevice& device)
        {
            {
                D3D12_COMMAND_QUEUE_DESC desc = {};
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 1;
                graphics_command_queue_ = device.CreateCommandQueue(desc);
            }

            {
                frame_contexts_.reserve(RenderSystemSettings::NUM_FRAMES_IN_FLIGHT);
                for (UINT i = 0; i < RenderSystemSettings::NUM_FRAMES_IN_FLIGHT; i++)
                    frame_contexts_.emplace_back(device);
            }

            {
                graphics_command_list_ = device.CreateGraphicsCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT,
                    frame_contexts_[0].command_allocator);
            }
        }

        UploadBuffer::Allocation CreateAndAllocateUploadBuffer(size_t size)
        {
            FrameContext& current_frame = frame_contexts_[current_frame_index_];

            UploadBuffer& upload_buffer = current_frame.CreateUploadBuffer(device_, size);

            return upload_buffer.Allocate(size, 1);
        }

        void ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER& pBarriers)
        {
            graphics_command_list_->ResourceBarrier(NumBarriers, &pBarriers);
        }

        void CopyBufferRegion(ID3D12Resource* pDstBuffer, UINT64 DstOffset, ID3D12Resource* pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes)
        {
            graphics_command_list_->CopyBufferRegion(pDstBuffer, DstOffset, pSrcBuffer, SrcOffset, NumBytes);
        }

    private:
        RenderDevice& device_;
        std::shared_ptr<ID3D12CommandQueue> graphics_command_queue_;
        std::shared_ptr<ID3D12GraphicsCommandList> graphics_command_list_;
        std::vector<FrameContext> frame_contexts_;
        size_t current_frame_index_ = 0;
    };
};