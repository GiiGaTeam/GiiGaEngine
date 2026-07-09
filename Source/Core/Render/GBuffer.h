#pragma once
#include<directxtk12/SimpleMath.h>

#include<memory>
#include<array>
#include<string>

#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<RenderContext.h>

namespace GiiGa
{
    /*
     *  GBuffer Structure:
     *                          R       G       B       A
     *      LightAccum          R       G       B    depth(copy)
     *      Diffuse             R       G       B      NU
     *      MatProps         Metal    Spec    Rough    Aniso
     *      NormalWS            X       Y       Z      NU
     *
     **** Depth/Stencil              D24_UNORM     S8_UINT
     */
    class GBuffer
    {
    public:
        enum class GBufferOrder
        {
            LightAccumulation,
            Diffuse,
            Material,
            NormalWS,
        };

        static inline const uint8_t NUM_GBUFFERS = 4;
        static inline const nvrhi::Format G_FORMAT = nvrhi::Format::RGBA32_FLOAT;
        static inline const nvrhi::Format DS_FORMAT = nvrhi::Format::D24S8;
        static inline const nvrhi::Color G_CLEAR = nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f);

        GBuffer(RenderDevice& device, DirectX::SimpleMath::Vector2 size):
            device_(device)
        {
            Resize(size);
        }

        nvrhi::IFramebuffer* GetFramebuffer() const { return framebuffer_; }

        // Framebuffer with only LightAccumulation (+ depth) — used by the lighting/forward/debug
        // passes that accumulate into LightAccumulation rather than the full G-buffer.
        nvrhi::IFramebuffer* GetLightFramebuffer() const { return light_framebuffer_; }

        // Depth-only framebuffer (no colour) — used by the light-volume "unmark" stencil pass.
        nvrhi::IFramebuffer* GetDepthOnlyFramebuffer() const { return depth_framebuffer_; }

        nvrhi::ITexture* GetColorTexture(GBufferOrder buffer) const
        {
            return resources_[static_cast<int>(buffer)];
        }

        nvrhi::ITexture* GetDepthTexture() const { return depth_res_; }

        void ClearAll(RenderContext& context) const
        {
            nvrhi::ICommandList* cl = context.GetCommandList();
            for (auto& tex : resources_)
                cl->clearTextureFloat(tex, nvrhi::AllSubresources, G_CLEAR);
            cl->clearDepthStencilTexture(depth_res_, nvrhi::AllSubresources, true, 1.0f, true, 1);
        }

        void ClearStencil(RenderContext& context, uint8_t value) const
        {
            context.GetCommandList()->clearDepthStencilTexture(depth_res_, nvrhi::AllSubresources, false, 0.0f, true, value);
        }

        void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            uint32_t w = static_cast<uint32_t>(new_size.x);
            uint32_t h = static_cast<uint32_t>(new_size.y);

            nvrhi::FramebufferDesc fbDesc;

            for (uint32_t i = 0; i < NUM_GBUFFERS; ++i)
            {
                nvrhi::TextureDesc td;
                td.width = w;
                td.height = h;
                td.format = G_FORMAT;
                td.dimension = nvrhi::TextureDimension::Texture2D;
                td.isRenderTarget = true;
                td.isShaderResource = true;
                td.setClearValue(G_CLEAR);
                td.initialState = nvrhi::ResourceStates::ShaderResource;
                td.keepInitialState = true;
                td.debugName = "GBuffer" + std::to_string(i);

                resources_[i] = device_.Device()->createTexture(td);
                fbDesc.addColorAttachment(resources_[i]);
            }

            {
                nvrhi::TextureDesc td;
                td.width = w;
                td.height = h;
                td.format = DS_FORMAT;
                td.dimension = nvrhi::TextureDimension::Texture2D;
                td.isRenderTarget = true;
                td.isShaderResource = true;
                td.setClearValue(nvrhi::Color(1.0f, 0.0f, 0.0f, 0.0f));
                td.initialState = nvrhi::ResourceStates::DepthWrite;
                td.keepInitialState = true;
                td.debugName = "GBufferDepth";

                depth_res_ = device_.Device()->createTexture(td);
                fbDesc.setDepthAttachment(depth_res_);
            }

            framebuffer_ = device_.Device()->createFramebuffer(fbDesc);

            light_framebuffer_ = device_.Device()->createFramebuffer(
                nvrhi::FramebufferDesc()
                .addColorAttachment(resources_[static_cast<int>(GBufferOrder::LightAccumulation)])
                .setDepthAttachment(depth_res_));

            depth_framebuffer_ = device_.Device()->createFramebuffer(
                nvrhi::FramebufferDesc().setDepthAttachment(depth_res_));
        }

    private:
        RenderDevice& device_;
        std::array<nvrhi::TextureHandle, NUM_GBUFFERS> resources_;
        nvrhi::TextureHandle depth_res_;
        nvrhi::FramebufferHandle framebuffer_;
        nvrhi::FramebufferHandle light_framebuffer_;
        nvrhi::FramebufferHandle depth_framebuffer_;
    };
}
