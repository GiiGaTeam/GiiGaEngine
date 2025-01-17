#pragma once


#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>

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

        virtual void Init(RenderContext& context) = 0;

        virtual ~Viewport() = default;

        virtual RenderPassViewData GetCameraInfo() =0;

        virtual void Execute(RenderContext& context) =0;

        virtual void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            viewport_size_ = new_size;

            gbuffer_->Resize(new_size);
        }

    protected:
        RenderDevice& device_;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<RenderGraph> renderGraph_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
