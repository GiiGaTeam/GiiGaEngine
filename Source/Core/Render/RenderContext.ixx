module;

#include<memory>
#include<vector>
#include <d3d12.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl2.h>

export module RenderContext;

import RenderSystemSettings;
import RenderDevice;
import FrameContext;
import Window;
import UploadBuffer;
import DescriptorHeap;
import SwapChain;

namespace GiiGa
{

    export class RenderContext final
    {
    public:
        RenderContext(RenderDevice& device):
            device_(device)
        {
            {
                D3D12_COMMAND_QUEUE_DESC desc = {};
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 1;
                graphics_command_queue_ = device_.CreateCommandQueue(desc);
            }

            {
                frame_contexts_.reserve(RenderSystemSettings::NUM_FRAMES_IN_FLIGHT);
                for (UINT i = 0; i < RenderSystemSettings::NUM_FRAMES_IN_FLIGHT; i++)
                    frame_contexts_.emplace_back(device_);
            }

            {
                graphics_command_list_ = device_.CreateGraphicsCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT,
                    frame_contexts_[0].command_allocator);
            }

            g_fence = device_.CreateFence(0, D3D12_FENCE_FLAG_NONE);

            g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            // Enable Gamepad Controls

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            GPUDescriptorHeap& descriptor_heap = device_.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            imgui_srv_desc_heap_allocation_ = descriptor_heap.Allocate(1);

            // Setup Platform/Renderer backends
            ImGui_ImplDX12_Init(device_.DetDevice().get(), RenderSystemSettings::NUM_FRAMES_IN_FLIGHT,
                DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap.GetDescriptorHeap().get(),
                imgui_srv_desc_heap_allocation_.GetCpuHandle(),
                imgui_srv_desc_heap_allocation_.GetGpuHandle());
        }

        void SetSwapChainWaitable(HANDLE waitable)
        {
            g_hSwapChainWaitableObject = waitable;
        }

        UploadBuffer::Allocation CreateAndAllocateUploadBuffer(size_t size)
        {
            UploadBuffer& upload_buffer = current_frame->CreateUploadBuffer(device_, size);

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

        std::shared_ptr<ID3D12CommandQueue> getGraphicsCommandQueue()
        {
            return graphics_command_queue_;
        }

        void Tick()
        {
            // Start the Dear ImGui frame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            bool show_demo_window = true;
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            ImGui::ShowDemoWindow(&show_demo_window);

            ImGui::Render();

            WaitForNextFrameResources();

            UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
            current_frame->Reset();

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            graphics_command_list_->Reset(frameCtx->CommandAllocator, nullptr);
            graphics_command_list_->ResourceBarrier(1, &barrier);

            // Render Dear ImGui graphics
            const float clear_color_with_alpha[4] = {0, 0, 0, 1};
            graphics_command_list_->ClearRenderTargetView(
                g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0,
                nullptr);
            graphics_command_list_->OMSetRenderTargets(
                1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
            graphics_command_list_->SetDescriptorHeaps(1,
                device_.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetDescriptorHeap().get());
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphics_command_list_.get());
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            graphics_command_list_->ResourceBarrier(1, &barrier);
            graphics_command_list_->Close();

            graphics_command_queue_->ExecuteCommandLists(
                1, (ID3D12CommandList* const*)&graphics_command_list_);
        }

        void WaitForNextFrameResources()
        {
            UINT nextFrameIndex = g_frameIndex + 1;
            g_frameIndex = nextFrameIndex;

            HANDLE waitableObjects[] = {g_hSwapChainWaitableObject, nullptr};
            DWORD numWaitableObjects = 1;

            current_frame = &frame_contexts_[
                nextFrameIndex % RenderSystemSettings::NUM_FRAMES_IN_FLIGHT];
            UINT64 fenceValue = current_frame->FenceValue;
            if (fenceValue != 0) // means no fence was signaled
            {
                current_frame->FenceValue = 0;
                g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
                waitableObjects[1] = g_fenceEvent;
                numWaitableObjects = 2;
            }

            WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);
        }

    private:
        RenderDevice& device_;
        std::shared_ptr<SwapChain> g_pSwapChain;
        DescriptorHeapAllocation imgui_srv_desc_heap_allocation_;
        std::shared_ptr<ID3D12CommandQueue> graphics_command_queue_;
        std::shared_ptr<ID3D12GraphicsCommandList> graphics_command_list_;
        std::vector<FrameContext> frame_contexts_;
        FrameContext* current_frame;
        std::shared_ptr<ID3D12Fence> g_fence;
        HANDLE g_fenceEvent;
        size_t g_frameIndex = 0;
        HANDLE g_hSwapChainWaitableObject = nullptr;
    };
};