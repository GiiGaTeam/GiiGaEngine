#pragma once

#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>

#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<RenderContext.h>
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

        virtual RTVData GetResultInfo() const { return rtvData_; }

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
            nvrhi::TextureDesc td;
            td.width = static_cast<uint32_t>(viewport_size_.x);
            td.height = static_cast<uint32_t>(viewport_size_.y);
            td.format = nvrhi::Format::RGBA32_FLOAT;
            td.dimension = nvrhi::TextureDimension::Texture2D;
            td.isRenderTarget = true;
            td.isShaderResource = true;
            td.setClearValue(nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
            td.initialState = nvrhi::ResourceStates::ShaderResource;
            td.keepInitialState = true;
            td.debugName = "ViewportResult";

            rtvData_.resource_ = device_.Device()->createTexture(td);
            rtvData_.framebuffer_ = device_.Device()->createFramebuffer(
                nvrhi::FramebufferDesc().addColorAttachment(rtvData_.resource_));
        }

    protected:
        RenderDevice& device_;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<RenderGraph> renderGraph_;
        std::shared_ptr<GBuffer> gbuffer_;

        RTVData rtvData_;
    };
}
