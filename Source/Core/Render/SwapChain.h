#pragma once


#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<dxgi1_6.h>
#include<vector>

#include<RenderSystemSettings.h>
#include<DirectXUtils.h>
#include<Window.h>
#include<RenderDevice.h>

namespace GiiGa
{
    // DXGI flip-model swap chain. The back buffers are used only by the editor's
    // final ImGui composition pass, which records native D3D12 commands, so they
    // are kept as raw resources with a small dedicated RTV heap (no NVRHI wrapping).
    class SwapChain
    {
    public:
        SwapChain(RenderDevice& device, const Window& window)
        {
            CreateRtvHeap(device);
            CreateSwapChainObject(device, window);
            CreateRenderTargets(device);
        }

        void Present()
        {
            swap_chain_->Present(1, 0);
        }

        HANDLE GetFrameLatencyWaitableObject()
        {
            return waitable_object_;
        }

        void Resize(RenderDevice& device, int width, int height)
        {
            back_buffers_.clear();
            swap_chain_->ResizeBuffers(RenderSystemSettings::NUM_BACK_BUFFERS, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
                                       DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            CreateRenderTargets(device);
        }

        // Transitions the current back buffer to RENDER_TARGET, clears it and binds it.
        void BeginRender(ID3D12GraphicsCommandList* cmd_list)
        {
            current_back_buffer_ = swap_chain_->GetCurrentBackBufferIndex();

            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = back_buffers_[current_back_buffer_].get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            cmd_list->ResourceBarrier(1, &barrier);

            const float clear_color[4] = {0, 0, 0, 1};
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRtv(current_back_buffer_);
            cmd_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
            cmd_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        }

        // Transitions the current back buffer back to PRESENT.
        void EndRender(ID3D12GraphicsCommandList* cmd_list)
        {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = back_buffers_[current_back_buffer_].get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            cmd_list->ResourceBarrier(1, &barrier);
        }

    private:
        UINT current_back_buffer_ = 0;
        std::shared_ptr<IDXGISwapChain4> swap_chain_;
        HANDLE waitable_object_ = nullptr;
        std::vector<std::shared_ptr<ID3D12Resource>> back_buffers_;

        std::shared_ptr<ID3D12DescriptorHeap> rtv_heap_;
        UINT rtv_descriptor_size_ = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE GetRtv(UINT index) const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += static_cast<SIZE_T>(index) * rtv_descriptor_size_;
            return handle;
        }

        void CreateRtvHeap(RenderDevice& device)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = RenderSystemSettings::NUM_BACK_BUFFERS;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            ID3D12DescriptorHeap* heap = nullptr;
            ThrowIfFailed(device.GetDxDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
            rtv_heap_ = std::shared_ptr<ID3D12DescriptorHeap>(heap, DXDeleter{});
            rtv_descriptor_size_ = device.GetDxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        void CreateSwapChainObject(RenderDevice& device, const Window& window)
        {
            DXGI_SWAP_CHAIN_DESC1 sd;
            {
                ZeroMemory(&sd, sizeof(sd));
                sd.BufferCount = RenderSystemSettings::NUM_BACK_BUFFERS;
                sd.Width = 0;
                sd.Height = 0;
                sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                sd.SampleDesc.Count = 1;
                sd.SampleDesc.Quality = 0;
                sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                sd.Scaling = DXGI_SCALING_STRETCH;
                sd.Stereo = FALSE;
            }

            IDXGIFactory4* dxgiFactory = nullptr;
            IDXGISwapChain1* swapChain1 = nullptr;
            IDXGISwapChain4* pSwapChain = nullptr;

            CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
            dxgiFactory->CreateSwapChainForHwnd(device.GetGraphicsQueue().get(), window.GetHandle(), &sd,
                                                nullptr, nullptr,
                                                &swapChain1);
            swapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain));

            swap_chain_ = std::shared_ptr<IDXGISwapChain4>(pSwapChain, DXDeleter{});

            swapChain1->Release();
            dxgiFactory->Release();

            swap_chain_->SetMaximumFrameLatency(RenderSystemSettings::NUM_BACK_BUFFERS);
            waitable_object_ = swap_chain_->GetFrameLatencyWaitableObject();
        }

        void CreateRenderTargets(RenderDevice& device)
        {
            back_buffers_.reserve(RenderSystemSettings::NUM_BACK_BUFFERS);
            for (UINT i = 0; i < RenderSystemSettings::NUM_BACK_BUFFERS; i++)
            {
                ID3D12Resource* pBackBuffer = nullptr;
                swap_chain_->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
                back_buffers_.emplace_back(pBackBuffer, DXDeleter{});
                device.GetDxDevice()->CreateRenderTargetView(pBackBuffer, nullptr, GetRtv(i));
            }
        }
    };
}
