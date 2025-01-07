module;

#include<directx/d3dx12.h>;
#include <d3d12.h>;

export module RenderContext;

import<memory>;
import<vector>;
import <queue>;

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
    struct DefferedUploadEntry
    {
        UploadBuffer uploadBuffer;
        UploadBuffer::Allocation allocation;
        DefferedUploadQuery query;
    };

    export class RenderContext : public IRenderContext
    {
        friend class EditorSwapChainPass;

    public:
        explicit RenderContext(RenderDevice& device):
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

        void CreateDefferedUpload(const std::span<const uint8_t>& data, DefferedUploadQuery&& query) override
        {
            DefferedUploadEntry entry
            {
                .uploadBuffer = UploadBuffer(device_, data.size_bytes()),
                .query = std::move(query)
            };

            entry.allocation = entry.uploadBuffer.Allocate(data.size(), 1);

            std::copy(data.begin(), data.end(), entry.allocation.CPU.begin());

            defferedUploads_.push(std::move(entry));
        }

        UploadBuffer::Allocation CreateAndAllocateUploadBufferInCurrentFrame(size_t size) override
        {
            UploadBuffer& upload_buffer = current_frame->CreateUploadBuffer(device_, size);

            return upload_buffer.Allocate(size, 1);
        }

        std::shared_ptr<BufferView<Constant>> AllocateDynamicConstantView(std::span<uint8_t> data, D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
        {
            if (!current_frame._Ptr) return nullptr;
            return current_frame->AllocateDynamicConstantView(device_, data, 256, desc);
        }

        std::shared_ptr<BufferView<ShaderResource>> AllocateDynamicShaderResourceView(std::span<uint8_t> data, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
        {
            return current_frame->AllocateDynamicShaderResourceView(device_, data, 16, desc);
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

            while (!defferedUploads_.empty())
            {
                DefferedUploadEntry defferedUpload = std::move(defferedUploads_.front());
                defferedUploads_.pop();

                CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    defferedUpload.query.destination.get(),
                    defferedUpload.query.getStateFunction(),
                    D3D12_RESOURCE_STATE_COPY_DEST
                );

                ResourceBarrier(1, barrier);

                CopyBufferRegion(defferedUpload.query.destination, 0, defferedUpload.allocation.resource, 0, defferedUpload.allocation.CPU.size());

                barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    defferedUpload.query.destination.get(),
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    defferedUpload.query.stateAfter
                );

                ResourceBarrier(1, barrier);

                defferedUpload.query.setStateFunction(defferedUpload.query.stateAfter);
            }

            ID3D12DescriptorHeap* descriptorHeap = device_.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetDescriptorHeap().
                                                           get();

            graphics_command_list_->SetDescriptorHeaps(1, &descriptorHeap);
        }

        void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CLEAR_VALUE clearValue)
        {
            graphics_command_list_->ClearRenderTargetView(rtv, clearValue.Color, 0, nullptr);
        }

        void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, D3D12_CLEAR_FLAGS flags, D3D12_CLEAR_VALUE clearValue)
        {
            graphics_command_list_->ClearDepthStencilView(dsv, flags, clearValue.DepthStencil.Depth, clearValue.DepthStencil.Stencil, 0, nullptr);
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

        void ContextIdle()
        {
            //current_frame = frame_contexts_.begin() + ++frameIndex_ % RenderSystemSettings::NUM_FRAMES_IN_FLIGHT;

            graphics_command_queue_->Signal(fence_.get(), fenceLastSignaledValue_ + 1);
            fence_->SetEventOnCompletion(current_frame->FenceValue + 1, fenceEvent_);
            WaitForSingleObject(fenceEvent_, INFINITE);
        }

        void WaitForNextFrameResources()
        {
            UINT nextFrameIndex = frameIndex_ + 1;
            frameIndex_ = nextFrameIndex;

            HANDLE waitableObjects[] = {frameLatencyWaitableObject_, nullptr};
            DWORD numWaitableObjects = 1;

            current_frame = frame_contexts_.begin() + nextFrameIndex % RenderSystemSettings::NUM_FRAMES_IN_FLIGHT;
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

        void BindPSO(ID3D12PipelineState* pPipelineState)
        {
            graphics_command_list_->SetPipelineState(pPipelineState);
        }

        void SetSignature(ID3D12RootSignature* pRootSignature)
        {
            graphics_command_list_->SetGraphicsRootSignature(pRootSignature);
        }

        void BindDescriptorHandle(UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle)
        {
            graphics_command_list_->SetGraphicsRootDescriptorTable(root_index, handle);
        }

        RenderDevice& GetDevice() { return device_; }
        std::shared_ptr<ID3D12GraphicsCommandList> GetGraphicsCommandList() { return graphics_command_list_; }

    private:
        RenderDevice& device_;
        std::shared_ptr<ID3D12CommandQueue> graphics_command_queue_;
        std::shared_ptr<ID3D12GraphicsCommandList> graphics_command_list_;
        std::vector<FrameContext> frame_contexts_;
        decltype(frame_contexts_)::iterator current_frame;
        std::shared_ptr<ID3D12Fence> fence_;
        HANDLE frameLatencyWaitableObject_;
        HANDLE fenceEvent_;
        size_t frameIndex_ = 0;
        UINT64 fenceLastSignaledValue_ = 0;
        std::queue<DefferedUploadEntry> defferedUploads_;
    };
};
