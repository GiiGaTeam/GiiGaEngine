module;

#include<memory>
#include<d3d12.h>
#include<dxgi1_6.h>
#include <vector>

export module SwapChain;

import RenderSystemSettings;
import DirectXUtils;
import Window;
import RenderDevice;
import BufferView;

namespace GiiGa
{
    export class SwapChain
    {
    public:
        SwapChain(RenderDevice& device, const std::shared_ptr<ID3D12CommandQueue>& command_queue, const Window& window)
        {
            CreateSwapChainObject(command_queue, window);

            render_target_resource.reserve(RenderSystemSettings::NUM_BACK_BUFFERS);
            render_target_views_.reserve(RenderSystemSettings::NUM_BACK_BUFFERS);

            CreateRenderTarget(device);
        }

        void Reset()
        {
            currentBackBufferIdx = swap_chain_->GetCurrentBackBufferIndex();
        }

        void Present()
        {
            swap_chain_->Present(1, 0);
        }

        HANDLE GetFrameLatencyWaitableObject()
        {
            return waitable_object_;
        }

        D3D12_RESOURCE_BARRIER getResourceBarrier(D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = render_target_resource[currentBackBufferIdx].get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = StateBefore;
            barrier.Transition.StateAfter = StateAfter;

            return barrier;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE getRTVDescriptorHandle()
        {
            return render_target_views_[currentBackBufferIdx].getDescriptor();
        }

    private:
        UINT currentBackBufferIdx = 0;
        std::shared_ptr<IDXGISwapChain4> swap_chain_;
        HANDLE waitable_object_ = nullptr;
        std::vector<std::shared_ptr<ID3D12Resource>> render_target_resource;
        std::vector<BufferView<RenderTarget>> render_target_views_;

        void CreateSwapChainObject(const std::shared_ptr<ID3D12CommandQueue> command_queue, const Window& window)
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
            dxgiFactory->CreateSwapChainForHwnd(command_queue.get(), window.GetHandle(), &sd,
                nullptr, nullptr,
                &swapChain1);
            swapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain));

            swap_chain_ = std::shared_ptr<IDXGISwapChain4>(pSwapChain, DirectXDeleter{});

            swapChain1->Release();
            dxgiFactory->Release();

            swap_chain_->SetMaximumFrameLatency(RenderSystemSettings::NUM_BACK_BUFFERS);
            waitable_object_ = swap_chain_->GetFrameLatencyWaitableObject();
        }

        void CreateRenderTarget(RenderDevice& device)
        {
            for (UINT i = 0; i < RenderSystemSettings::NUM_BACK_BUFFERS; i++)
            {
                ID3D12Resource* pBackBuffer = nullptr;
                swap_chain_->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
                auto res = std::shared_ptr<ID3D12Resource>(pBackBuffer, DirectXDeleter());
                render_target_resource.push_back(res);
                render_target_views_.push_back(device.CreateRenderTargetView(res, nullptr));
            }
        }
    };
}