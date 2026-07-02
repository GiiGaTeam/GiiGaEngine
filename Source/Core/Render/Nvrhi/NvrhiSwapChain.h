#pragma once

// NvrhiSwapChain.h
//
// DXGI flip-model swap chain whose back buffers are exposed as NVRHI textures and
// framebuffers. This is genuine stateful logic (it owns the DXGI swap chain, wraps each
// back buffer as an NVRHI texture via createHandleForNativeTexture, and owns one
// framebuffer per back buffer), so it earns a class — unlike the device/command list,
// which are used directly as nvrhi handles.
//
// Back buffers are created with keepInitialState=true and initialState=Present, so NVRHI
// assumes each command list sees them in Present and transitions them back to Present on
// close — exactly what flip-discard presentation requires.

#include "NvrhiCommon.h"

#include <vector>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

namespace GiiGa::rhi
{
    class SwapChain
    {
    public:
        SwapChain(nvrhi::IDevice* nvrhiDevice, ID3D12CommandQueue* graphicsQueue, HWND hwnd,
                  uint32_t width = 0, uint32_t height = 0, uint32_t bufferCount = 3)
            : nvrhiDevice_(nvrhiDevice)
            , bufferCount_(bufferCount)
            , width_(width)
            , height_(height)
        {
            if (!nvrhiDevice_ || !graphicsQueue || !hwnd)
            {
                throw std::runtime_error("rhi::SwapChain: null argument");
            }

            createSwapChainObject(graphicsQueue, hwnd);
            wrapBackBuffers();
        }

        ~SwapChain()
        {
            framebuffers_.clear();
            textures_.clear();
            swapChain_.Reset();
        }

        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        // Advance to the current back buffer index for this frame.
        void beginFrame()
        {
            currentIndex_ = swapChain_->GetCurrentBackBufferIndex();
        }

        nvrhi::ITexture* getCurrentTexture() const
        {
            return currentIndex_ < textures_.size() ? textures_[currentIndex_].Get() : nullptr;
        }

        nvrhi::IFramebuffer* getCurrentFramebuffer() const
        {
            return currentIndex_ < framebuffers_.size() ? framebuffers_[currentIndex_].Get() : nullptr;
        }

        nvrhi::Format getFormat() const { return nvrhi::Format::RGBA8_UNORM; }
        uint32_t getWidth() const { return width_; }
        uint32_t getHeight() const { return height_; }
        uint32_t getCurrentIndex() const { return currentIndex_; }
        uint32_t getBufferCount() const { return bufferCount_; }

        void present(uint32_t syncInterval = 1)
        {
            swapChain_->Present(syncInterval, 0);
        }

        HANDLE getFrameLatencyWaitableObject() const { return waitableObject_; }

        void resize(uint32_t width, uint32_t height)
        {
            if (width == width_ && height == height_) return;

            // Drop NVRHI views first, then resize the DXGI buffers, then re-wrap.
            framebuffers_.clear();
            textures_.clear();

            width_ = width;
            height_ = height;

            ThrowIfFailed(swapChain_->ResizeBuffers(bufferCount_, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
                                                    DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT),
                          "IDXGISwapChain::ResizeBuffers");
            wrapBackBuffers();
        }

    private:
        nvrhi::IDevice* nvrhiDevice_ = nullptr;
        uint32_t bufferCount_ = 3;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t currentIndex_ = 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
        HANDLE waitableObject_ = nullptr;
        std::vector<nvrhi::TextureHandle> textures_;
        std::vector<nvrhi::FramebufferHandle> framebuffers_;

        void createSwapChainObject(ID3D12CommandQueue* graphicsQueue, HWND hwnd)
        {
            Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
            UINT flags = 0;
#ifdef _DEBUG
            flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
            if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory))))
            {
                ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory");
            }

            DXGI_SWAP_CHAIN_DESC1 sd{};
            sd.Width = width_;
            sd.Height = height_;
            sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.SampleDesc.Count = 1;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.BufferCount = bufferCount_;
            sd.Scaling = DXGI_SCALING_STRETCH;
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
            ThrowIfFailed(factory->CreateSwapChainForHwnd(graphicsQueue, hwnd, &sd, nullptr, nullptr, &swapChain1),
                          "CreateSwapChainForHwnd");
            ThrowIfFailed(swapChain1.As(&swapChain_), "QueryInterface IDXGISwapChain4");

            swapChain_->SetMaximumFrameLatency(bufferCount_);
            waitableObject_ = swapChain_->GetFrameLatencyWaitableObject();

            if (width_ == 0 || height_ == 0)
            {
                DXGI_SWAP_CHAIN_DESC1 cur{};
                swapChain_->GetDesc1(&cur);
                width_ = cur.Width;
                height_ = cur.Height;
            }
        }

        void wrapBackBuffers()
        {
            textures_.reserve(bufferCount_);
            framebuffers_.reserve(bufferCount_);

            for (uint32_t i = 0; i < bufferCount_; ++i)
            {
                Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
                ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffer)), "GetBuffer");

                nvrhi::TextureDesc td;
                td.setWidth(width_)
                  .setHeight(height_)
                  .setFormat(nvrhi::Format::RGBA8_UNORM)
                  .setIsRenderTarget(true)
                  .setInitialState(nvrhi::ResourceStates::Present)
                  .setKeepInitialState(true)
                  .setDebugName("SwapChain back buffer");

                nvrhi::TextureHandle tex = nvrhiDevice_->createHandleForNativeTexture(
                    nvrhi::ObjectTypes::D3D12_Resource,
                    nvrhi::Object(static_cast<void*>(backBuffer.Get())),
                    td);
                backBuffer.Reset();

                nvrhi::FramebufferDesc fbd;
                fbd.addColorAttachment(tex);
                nvrhi::FramebufferHandle fb = nvrhiDevice_->createFramebuffer(fbd);

                textures_.push_back(tex);
                framebuffers_.push_back(fb);
            }
        }
    };
}
