module;

#include<memory>
#include<vector>
#include <d3d12.h>
#include <span>

export module RenderContext;

import IRenderContext;
import RenderSystemSettings;
import RenderDevice;
import FrameContext;
import Window;
import DescriptorHeap;
import SwapChain;
import BufferView;

namespace GiiGa
{
    export class RenderContext : public IRenderContext
    {
        friend class EditorSwapChainPass;

    public:
        RenderContext(RenderDevice& device):
            device_(device),
            graphics_command_queue_([&device]()
            {
                D3D12_COMMAND_QUEUE_DESC desc{};
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 1;
                return device.CreateCommandQueue(desc);
            }())
        {
            {
                frame_contexts_.reserve(RenderSystemSettings::NUM_FRAMES_IN_FLIGHT);
                for (UINT i = 0; i < RenderSystemSettings::NUM_FRAMES_IN_FLIGHT; i++)
                    frame_contexts_.emplace_back(device_);
            }

            {
                graphics_command_list_ = device_.CreateGraphicsCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                                           frame_contexts_[0].command_allocator);
                graphics_command_list_->Close();
            }

            fence_ = device_.CreateFence(0, D3D12_FENCE_FLAG_NONE);

            fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }

        void SetFrameLatencyWaitableObject(HANDLE FrameLatencyWaitableObject)
        {
            frameLatencyWaitableObject_ = FrameLatencyWaitableObject;
        }

        UploadBuffer::Allocation CreateAndAllocateUploadBuffer(size_t size) override
        {
            UploadBuffer& upload_buffer = current_frame->CreateUploadBuffer(device_, size);

            return upload_buffer.Allocate(size, 1);
        }

        std::shared_ptr<BufferView<Constant>> AllocateDynamicConstantView(std::span<uint8_t> data, size_t alignment,
                                                                          D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
        {
            return current_frame->AllocateDynamicConstantView(device_, data, alignment, desc);
        }

        std::shared_ptr<BufferView<ShaderResource>> AllocateDynamicShaderResourceView(std::span<uint8_t> data, size_t alignment,
                                                                                      const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
        {
            return current_frame->AllocateDynamicShaderResourceView(device_, data, alignment, desc);
        }

        void ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER& pBarriers)
        {
            graphics_command_list_->ResourceBarrier(NumBarriers, &pBarriers);
        }

        void CopyBufferRegion(const std::shared_ptr<ID3D12Resource>& pDstBuffer, UINT64 DstOffset,
                              const std::shared_ptr<ID3D12Resource>& pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) override
        {
            graphics_command_list_->CopyBufferRegion(pDstBuffer.get(), DstOffset, pSrcBuffer.get(), SrcOffset, NumBytes);
        }

        std::shared_ptr<ID3D12CommandQueue> getGraphicsCommandQueue()
        {
            return graphics_command_queue_;
        }

        void StartFrame()
        {
            WaitForNextFrameResources();

            current_frame->Reset(graphics_command_list_);

            ID3D12DescriptorHeap* descriptorHeap = device_.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetDescriptorHeap().
                                                           get();

            graphics_command_list_->SetDescriptorHeaps(1, &descriptorHeap);
        }

        void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4])
        {
            graphics_command_list_->ClearRenderTargetView(rtv, color, 0, nullptr);
        }

        void EndFrame()
        {
            graphics_command_list_->Close();

            graphics_command_queue_->ExecuteCommandLists(
                1, (ID3D12CommandList* const*)&graphics_command_list_);

            UINT64 fenceValue = fenceLastSignaledValue_ + 1;
            graphics_command_queue_->Signal(fence_.get(), fenceValue);
            fenceLastSignaledValue_ = fenceValue;
            current_frame->FenceValue = fenceValue;
        }

        void WaitForNextFrameResources()
        {
            UINT nextFrameIndex = frameIndex_ + 1;
            frameIndex_ = nextFrameIndex;

            HANDLE waitableObjects[] = {frameLatencyWaitableObject_, nullptr};
            DWORD numWaitableObjects = 1;

            current_frame = &frame_contexts_[
                nextFrameIndex % RenderSystemSettings::NUM_FRAMES_IN_FLIGHT];
            UINT64 fenceValue = current_frame->FenceValue;
            if (fenceValue != 0) // means no fence was signaled
            {
                current_frame->FenceValue = 0;
                fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
                waitableObjects[1] = fenceEvent_;
                numWaitableObjects = 2;
            }

            WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);
        }

    private:
        RenderDevice& device_;
        std::shared_ptr<ID3D12CommandQueue> graphics_command_queue_;
        std::shared_ptr<ID3D12GraphicsCommandList> graphics_command_list_;
        std::vector<FrameContext> frame_contexts_;
        FrameContext* current_frame;
        std::shared_ptr<ID3D12Fence> fence_;
        HANDLE frameLatencyWaitableObject_;
        HANDLE fenceEvent_;
        size_t frameIndex_ = 0;
        UINT64 fenceLastSignaledValue_ = 0;
    };
};
