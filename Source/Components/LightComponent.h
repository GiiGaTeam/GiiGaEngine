#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directxtk12/SimpleMath.h>

#include<memory>
#include<vector>

#include<nvrhi/nvrhi.h>

#include<Component.h>
#include<IRenderable.h>
#include<TransformComponent.h>
#include<IUpdateGPUData.h>
#include<RenderDevice.h>
#include<RenderContext.h>

namespace GiiGa
{
    // Abstract base for lights. Owns the shared shadow-map infrastructure (an NVRHI depth
    // texture + a depth-only framebuffer). Concrete lights fill these in InitShadowData.
    class LightComponent : public Component, public IRenderable, public IUpdateGPUData
    {
    public:
        LightComponent() = default;

        LightComponent(const Json::Value& json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        virtual void InitShadowData(RenderDevice& device) = 0;

        nvrhi::ITexture* GetShadowTexture() const { return shadow_texture_; }
        nvrhi::IFramebuffer* GetShadowFramebuffer() const { return shadow_framebuffer_; }

        virtual std::vector<DirectX::SimpleMath::Matrix> GetViews() { return shadow_views; }

    protected:
        std::weak_ptr<TransformComponent> transform_;

        DirectX::SimpleMath::Vector2 TEXTURE_SIZE = DirectX::SimpleMath::Vector2(800);

        std::vector<DirectX::SimpleMath::Matrix> shadow_views;
        nvrhi::TextureHandle shadow_texture_;
        nvrhi::FramebufferHandle shadow_framebuffer_;
    };
}
