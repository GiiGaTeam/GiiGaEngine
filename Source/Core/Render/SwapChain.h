#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<dxgi1_6.h>
#include<vector>
#include<memory>
#include<string>

#include<nvrhi/nvrhi.h>
#include<nvrhi/d3d12.h>

#include<RenderSystemSettings.h>
#include<RenderTypes.h>
#include<DirectXUtils.h>
#include<Window.h>
#include<RenderDevice.h>

namespace GiiGa
{
    // DXGI flip-model swap chain (NVRHI does not manage swap chains). Each back buffer is
    // wrapped as an nvrhi::TextureHandle via createHandleForNativeTexture, plus a
    // per-back-buffer nvrhi::FramebufferHandle for use as a render target.
    class SwapChain
    {
    public:
        SwapChain(RenderDevice& device, const Window& window):
            device_(device)
        {
            CreateSwapChainObject(device, window);
            CreateRenderTargets(device);
        }

        void Present()
        {
            swap_chain_->Present(1, 0);
        }

        HANDLE GetFrameLatencyWaitableObject() const { return waitable_object_; }

        uint32_t GetCurrentBackBufferIndex() const { return swap_chain_->GetCurrentBackBufferIndex(); }

        nvrhi::ITexture* GetCurrentBackBufferTexture()
        {
            return back_buffers_[swap_chain_->GetCurrentBackBufferIndex()];
        }

        nvrhi::IFramebuffer* GetCurrentFramebuffer()
        {
            return framebuffers_[swap_chain_->GetCurrentBackBufferIndex()];
        }

        // CPU RTV descriptor of the current back buffer (used by imgui_impl_dx12's OMSetRenderTargets).
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV()
        {
            nvrhi::ITexture* tex = GetCurrentBackBufferTexture();
            nvrhi::Object obj = tex->getNativeView(nvrhi::ObjectTypes::D3D12_RenderTargetViewDescriptor,
                                                   nvrhi::Format::UNKNOWN, nvrhi::AllSubresources,
                                                   nvrhi::TextureDimension::Texture2D);
            D3D12_CPU_DESCRIPTOR_HANDLE handle{};
            handle.ptr = obj.integer;
            return handle;
        }

        void Resize(RenderDevice& device, int width, int height)
        {
            framebuffers_.clear();
            back_buffers_.clear();

            ThrowIfFailed(swap_chain_->ResizeBuffers(RenderSystemSettings::NUM_BACK_BUFFERS, width, height,
                                                     RenderSystemSettings::BACK_BUFFER_FORMAT,
                                                     DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
            CreateRenderTargets(device);
        }

    private:
        RenderDevice& device_;
        std::shared_ptr<IDXGISwapChain4> swap_chain_;
        HANDLE waitable_object_ = nullptr;
        std::vector<nvrhi::TextureHandle> back_buffers_;
        std::vector<nvrhi::FramebufferHandle> framebuffers_;

        void CreateSwapChainObject(RenderDevice& device, const Window& window)
        {
            DXGI_SWAP_CHAIN_DESC1 sd;
            ZeroMemory(&sd, sizeof(sd));
            sd.BufferCount = RenderSystemSettings::NUM_BACK_BUFFERS;
            sd.Width = 0;
            sd.Height = 0;
            sd.Format = RenderSystemSettings::BACK_BUFFER_FORMAT;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            sd.Scaling = DXGI_SCALING_STRETCH;
            sd.Stereo = FALSE;

            IDXGIFactory4* dxgiFactory = nullptr;
            IDXGISwapChain1* swapChain1 = nullptr;
            IDXGISwapChain4* pSwapChain = nullptr;

            CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
            dxgiFactory->CreateSwapChainForHwnd(device.GetD3D12Queue(), window.GetHandle(), &sd,
                                                nullptr, nullptr, &swapChain1);
            swapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain));

            swap_chain_ = std::shared_ptr<IDXGISwapChain4>(pSwapChain, DXDeleter{});

            swapChain1->Release();
            dxgiFactory->Release();

            swap_chain_->SetMaximumFrameLatency(RenderSystemSettings::NUM_BACK_BUFFERS);
            waitable_object_ = swap_chain_->GetFrameLatencyWaitableObject();
        }

        void CreateRenderTargets(RenderDevice& device)
        {
            for (UINT i = 0; i < RenderSystemSettings::NUM_BACK_BUFFERS; i++)
            {
                Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
                swap_chain_->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

                D3D12_RESOURCE_DESC rd = backBuffer->GetDesc();

                nvrhi::TextureDesc texDesc;
                texDesc.width = static_cast<uint32_t>(rd.Width);
                texDesc.height = rd.Height;
                texDesc.format = ToNvrhiFormat(RenderSystemSettings::BACK_BUFFER_FORMAT);
                texDesc.dimension = nvrhi::TextureDimension::Texture2D;
                texDesc.isRenderTarget = true;
                texDesc.debugName = "BackBuffer" + std::to_string(i);
                texDesc.initialState = nvrhi::ResourceStates::Present;
                texDesc.keepInitialState = true;

                nvrhi::TextureHandle tex = device.Device()->createHandleForNativeTexture(
                    nvrhi::ObjectTypes::D3D12_Resource, nvrhi::Object(backBuffer.Get()), texDesc);

                nvrhi::FramebufferHandle fb = device.Device()->createFramebuffer(
                    nvrhi::FramebufferDesc().addColorAttachment(tex));

                back_buffers_.push_back(tex);
                framebuffers_.push_back(fb);
            }
        }
    };
}
