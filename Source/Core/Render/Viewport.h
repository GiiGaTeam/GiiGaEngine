#pragma once


#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<RenderPassViewData.h>
#include<GBuffer.h>

namespace GiiGa
{
    class RenderGraph;

    class Viewport
    {
    public:
        explicit Viewport(RenderDevice& device):
            device_(device),
            gbuffer_(std::make_shared<GBuffer>(device, viewport_size_))
        {
        }

        virtual void Init(RenderContext& context)
        {
            CreateRenderTargetResult();
        }

        virtual ~Viewport() = default;

        virtual RenderPassViewData GetCameraInfo() =0;

        virtual RTVData GetResultInfo() const { return rtvData_; };

        virtual void Execute(RenderContext& context) =0;

        virtual void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            viewport_size_ = new_size;

            gbuffer_->Resize(new_size);

            CreateRenderTargetResult();
        }

        virtual void SetCamera(std::shared_ptr<CameraComponent> camera) =0;

        virtual void CreateRenderTargetResult()
        {
            nvrhi::TextureDesc desc;
            desc.width = std::max(1u, static_cast<uint32_t>(viewport_size_.x));
            desc.height = std::max(1u, static_cast<uint32_t>(viewport_size_.y));
            desc.format = nvrhi::Format::RGBA32_FLOAT;
            desc.isRenderTarget = true;
            desc.initialState = nvrhi::ResourceStates::RenderTarget;
            desc.keepInitialState = true;
            desc.clearValue = nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f);
            desc.useClearValue = true;
            desc.debugName = "ViewportResult";

            rtvData_.texture = device_.NvDevice()->createTexture(desc);

            nvrhi::FramebufferDesc fb_desc;
            fb_desc.addColorAttachment(rtvData_.texture);
            rtvData_.framebuffer = device_.NvDevice()->createFramebuffer(fb_desc);
        }

    protected:
        RenderDevice& device_;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<RenderGraph> renderGraph_;
        std::shared_ptr<GBuffer> gbuffer_;

        RTVData rtvData_;
    };
}
