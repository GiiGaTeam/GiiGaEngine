#pragma once
#include<directxtk12/SimpleMath.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>

#include<memory>
#include<array>

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

        GBuffer(RenderDevice& device, DirectX::SimpleMath::Vector2 size):
            device_(device)
        {
            Resize(size);
        }

        void ClearAll(RenderContext& context)
        {
            for (auto& resource : resources_)
            {
                context.CommandList()->clearTextureFloat(resource, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
            }
            context.CommandList()->clearDepthStencilTexture(depth_res_, nvrhi::AllSubresources, true, 1.0f, true, 1);
        }

        void ClearStencil(RenderContext& context, uint8_t value)
        {
            context.CommandList()->clearDepthStencilTexture(depth_res_, nvrhi::AllSubresources, false, 0.0f, true, value);
        }

        nvrhi::ITexture* GetTexture(GBufferOrder buffer) const
        {
            return resources_[static_cast<int>(buffer)];
        }

        nvrhi::ITexture* GetDepthTexture() const
        {
            return depth_res_;
        }

        // All four color targets + depth (geometry pass).
        nvrhi::IFramebuffer* GetGeometryFramebuffer() const { return geometry_framebuffer_; }

        // LightAccumulation + depth (lighting / forward / debug passes).
        nvrhi::IFramebuffer* GetAccumFramebuffer() const { return accum_framebuffer_; }

        // Depth only (stencil marking in the lighting pass).
        nvrhi::IFramebuffer* GetDepthOnlyFramebuffer() const { return depth_framebuffer_; }

        void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            const uint32_t width = std::max(1u, static_cast<uint32_t>(new_size.x));
            const uint32_t height = std::max(1u, static_cast<uint32_t>(new_size.y));

            static const char* debug_names[NUM_GBUFFERS] = {"GBuffer.LightAccum", "GBuffer.Diffuse", "GBuffer.Material", "GBuffer.NormalWS"};

            for (uint32_t i = 0; i < NUM_GBUFFERS; ++i)
            {
                nvrhi::TextureDesc desc;
                desc.width = width;
                desc.height = height;
                desc.format = G_FORMAT;
                desc.isRenderTarget = true;
                desc.initialState = nvrhi::ResourceStates::RenderTarget;
                desc.keepInitialState = true;
                desc.clearValue = nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f);
                desc.useClearValue = true;
                desc.debugName = debug_names[i];

                resources_[i] = device_.NvDevice()->createTexture(desc);
            }

            {
                nvrhi::TextureDesc desc;
                desc.width = width;
                desc.height = height;
                desc.format = DS_FORMAT;
                desc.isRenderTarget = true;
                desc.initialState = nvrhi::ResourceStates::DepthWrite;
                desc.keepInitialState = true;
                desc.clearValue = nvrhi::Color(1.0f, 1.0f, 0.0f, 0.0f);
                desc.useClearValue = true;
                desc.debugName = "GBuffer.DepthStencil";

                depth_res_ = device_.NvDevice()->createTexture(desc);
            }

            {
                nvrhi::FramebufferDesc desc;
                for (uint32_t i = 0; i < NUM_GBUFFERS; ++i)
                    desc.addColorAttachment(resources_[i]);
                desc.setDepthAttachment(depth_res_);
                geometry_framebuffer_ = device_.NvDevice()->createFramebuffer(desc);
            }

            {
                nvrhi::FramebufferDesc desc;
                desc.addColorAttachment(resources_[static_cast<int>(GBufferOrder::LightAccumulation)]);
                desc.setDepthAttachment(depth_res_);
                accum_framebuffer_ = device_.NvDevice()->createFramebuffer(desc);
            }

            {
                nvrhi::FramebufferDesc desc;
                desc.setDepthAttachment(depth_res_);
                depth_framebuffer_ = device_.NvDevice()->createFramebuffer(desc);
            }
        }

    private:
        RenderDevice& device_;
        std::array<nvrhi::TextureHandle, NUM_GBUFFERS> resources_;
        nvrhi::TextureHandle depth_res_;

        nvrhi::FramebufferHandle geometry_framebuffer_;
        nvrhi::FramebufferHandle accum_framebuffer_;
        nvrhi::FramebufferHandle depth_framebuffer_;
    };
}
