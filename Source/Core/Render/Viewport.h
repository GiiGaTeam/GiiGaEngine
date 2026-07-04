#pragma once

// Viewport.h — abstract base for a renderable viewport.
// NVRHI cutover: owns a rhi::GBuffer (4 RGBA32F + D24S8) and an off-screen result texture
// (the viewport's final image, sampled by ImGui). The constructor signature is preserved
// (RenderDevice&) so the legacy EditorRenderSystem wiring stays unchanged; the NVRHI device
// is supplied by the concrete subclass (e.g. EditorViewport pulls it from the RenderSystem)
// and the GBuffer / result texture are created lazily in ensureResources() once it is set.

#include <memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directxtk12/SimpleMath.h>

#include <RenderDevice.h>
#include <RenderPassViewData.h>
#include <Core/Render/Nvrhi/NvrhiGBuffer.h>

namespace GiiGa
{
    class RenderGraph;

    class Viewport
    {
    public:
        explicit Viewport(RenderDevice& device)
            : device_(device)
        {
        }

        virtual ~Viewport() = default;

        virtual void Init(RenderContext& /*context*/)
        {
            ensureResources();
        }

        virtual RenderPassViewData GetCameraInfo() = 0;

        virtual RTVData GetResultInfo() const { return rtvData_; };

        virtual void Execute(RenderContext& context) = 0;

        virtual void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            viewport_size_ = new_size;
            ensureResources();
            if (gbuffer_)
                gbuffer_->resize(static_cast<uint32_t>(new_size.x), static_cast<uint32_t>(new_size.y));
            CreateRenderTargetResult();
        }

        virtual void SetCamera(std::shared_ptr<CameraComponent> camera) = 0;

        // Lazily create the GBuffer + result texture once the NVRHI device is known.
        void ensureResources()
        {
            if (!nvrhiDevice_) return;
            if (!gbuffer_)
            {
                gbuffer_ = std::make_shared<rhi::GBuffer>(nvrhiDevice_,
                                                          static_cast<uint32_t>(viewport_size_.x),
                                                          static_cast<uint32_t>(viewport_size_.y));
                CreateRenderTargetResult();
            }
        }

        virtual void CreateRenderTargetResult()
        {
            if (!nvrhiDevice_) return;

            nvrhi::TextureDesc td;
            td.setWidth(static_cast<uint32_t>(viewport_size_.x))
              .setHeight(static_cast<uint32_t>(viewport_size_.y))
              .setFormat(nvrhi::Format::RGBA32_FLOAT)
              .setIsRenderTarget(true)
              .setInitialState(nvrhi::ResourceStates::RenderTarget)
              .setKeepInitialState(true)
              .setDebugName("viewport result");
            rtvData_.resource = nvrhiDevice_->createTexture(td);

            nvrhi::FramebufferDesc fbd;
            fbd.addColorAttachment(rtvData_.resource);
            resultFramebuffer_ = nvrhiDevice_->createFramebuffer(fbd);
        }

    protected:
        RenderDevice& device_;
        nvrhi::IDevice* nvrhiDevice_ = nullptr;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<rhi::GBuffer> gbuffer_;
        nvrhi::FramebufferHandle resultFramebuffer_;

        RTVData rtvData_;
    };
}
