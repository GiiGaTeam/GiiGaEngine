#pragma once

// NvrhiGBuffer.h
//
// NVRHI-backed geometry buffer for the deferred pipeline: four RGBA32F color render
// targets (LightAccumulation, Diffuse, Material, NormalWS) plus a D24S8 depth/stencil,
// exposed as nvrhi::TextureHandle and bundled into one nvrhi::FramebufferHandle.
//
// This is the NVRHI replacement for the legacy GBuffer.h, which manually created
// ID3D12Resource + RTV/SRV/DSV descriptor views and drove D3D12_RESOURCE_STATES transitions.
// NVRHI creates views lazily and places barriers automatically, so the whole view/state
// machinery collapses to "create textures, make a framebuffer, clear".

#include "NvrhiCommon.h"

namespace GiiGa::rhi
{
    class GBuffer
    {
    public:
        enum class Order
        {
            LightAccumulation = 0,
            Diffuse = 1,
            Material = 2,
            NormalWS = 3,
        };

        static constexpr uint32_t NUM_COLORS = 4;
        static constexpr nvrhi::Format kColorFormat = nvrhi::Format::RGBA32_FLOAT;
        static constexpr nvrhi::Format kDepthFormat = nvrhi::Format::D24S8;

        GBuffer(nvrhi::IDevice* device, uint32_t width, uint32_t height)
            : device_(device)
        {
            resize(width, height);
        }

        void resize(uint32_t width, uint32_t height)
        {
            if (width_ == width && height_ == height && framebuffer_) return;
            width_ = width;
            height_ = height;

            colorTextures_.clear();

            nvrhi::FramebufferDesc fbd;

            for (uint32_t i = 0; i < NUM_COLORS; ++i)
            {
                nvrhi::TextureDesc td;
                td.setWidth(width_)
                  .setHeight(height_)
                  .setFormat(kColorFormat)
                  .setIsRenderTarget(true)
                  .setInitialState(nvrhi::ResourceStates::RenderTarget)
                  .setDebugName(colorName(i));
                nvrhi::TextureHandle tex = device_->createTexture(td);
                colorTextures_.push_back(tex);
                fbd.addColorAttachment(tex);
            }

            {
                nvrhi::TextureDesc td;
                td.setWidth(width_)
                  .setHeight(height_)
                  .setFormat(kDepthFormat)
                  .setInitialState(nvrhi::ResourceStates::DepthWrite)
                  .setDebugName("GBuffer depth");
                depthTexture_ = device_->createTexture(td);
                fbd.setDepthAttachment(depthTexture_);
            }

            framebuffer_ = device_->createFramebuffer(fbd);
        }

        nvrhi::IFramebuffer* getFramebuffer() const { return framebuffer_.Get(); }
        nvrhi::ITexture* getColor(Order o) const { return colorTextures_[static_cast<size_t>(o)].Get(); }
        nvrhi::ITexture* getColor(uint32_t index) const { return colorTextures_[index].Get(); }
        nvrhi::ITexture* getDepth() const { return depthTexture_.Get(); }
        nvrhi::Format colorFormat() const { return kColorFormat; }
        nvrhi::Format depthFormat() const { return kDepthFormat; }
        uint32_t width() const { return width_; }
        uint32_t height() const { return height_; }

        // Clear all attachments. Call inside an open command list, before rendering.
        void clearAll(nvrhi::ICommandList* cmd)
        {
            const nvrhi::Color clearColor{0.0f, 0.0f, 0.0f, 1.0f};
            for (uint32_t i = 0; i < NUM_COLORS; ++i)
            {
                cmd->clearTextureFloat(colorTextures_[i].Get(), nvrhi::AllSubresources, clearColor);
            }
            cmd->clearDepthStencilTexture(depthTexture_.Get(), nvrhi::AllSubresources,
                                          /*clearDepth=*/true, 1.0f, /*clearStencil=*/true, 0);
        }

    private:
        nvrhi::IDevice* device_ = nullptr;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        std::vector<nvrhi::TextureHandle> colorTextures_;
        nvrhi::TextureHandle depthTexture_;
        nvrhi::FramebufferHandle framebuffer_;

        static const char* colorName(uint32_t i)
        {
            switch (i)
            {
            case 0: return "GBuffer LightAccumulation";
            case 1: return "GBuffer Diffuse";
            case 2: return "GBuffer Material";
            default: return "GBuffer NormalWS";
            }
        }
    };
}
