module;

#include<memory>
#include<d3d12.h>
#include<dxgi1_6.h>

export module SwapChain;

import RenderSystemSettings;
import DirectXUtils;

namespace GiiGa
{
    export class SwapChain
    {
    public:
        void Create(const std::shared_ptr<ID3D12CommandQueue>& command_queue, const Window& window)
        {
            CreateSwapChainObject(command_queue,window);

            swap_chain_->SetMaximumFrameLatency(RenderSystemSettings::NUM_BACK_BUFFERS);

            waitable_object_ = swap_chain_->GetFrameLatencyWaitableObject();
            
        }

    private:
        std::shared_ptr<IDXGISwapChain4> swap_chain_;
        HANDLE waitable_object_;

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
        }

        
    };
}