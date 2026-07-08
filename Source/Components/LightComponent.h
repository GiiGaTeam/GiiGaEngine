#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directxtk12/SimpleMath.h>
#include <nvrhi/nvrhi.h>

#include<memory>

#include<Component.h>
#include<Material.h>
#include<IRenderable.h>
#include<TransformComponent.h>
#include<IUpdateGPUData.h>
#include<RenderDevice.h>
#include<RenderContext.h>

namespace GiiGa
{
    class LightComponent : public Component, public IRenderable, public IUpdateGPUData
    {
    public:
        LightComponent() = default;

        LightComponent(const Json::Value& json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        virtual void InitShadowData(RenderDevice& device) = 0;

        virtual void ClearShadowMap(RenderContext& context)
        {
            context.CommandList()->clearDepthStencilTexture(shadow_map_, nvrhi::AllSubresources, true, 1.0f, false, 0);
        }

        virtual nvrhi::ITexture* GetShadowMap() const
        {
            return shadow_map_;
        }

        virtual nvrhi::IFramebuffer* GetShadowFramebuffer() const
        {
            return shadow_framebuffer_;
        }

        virtual std::vector<DirectX::SimpleMath::Matrix> GetViews()
        {
            return shadow_views;
        }

        DirectX::SimpleMath::Vector2 GetShadowMapSize() const
        {
            return TEXTURE_SIZE;
        }

    protected:
        std::weak_ptr<TransformComponent> transform_;

        DirectX::SimpleMath::Vector2 TEXTURE_SIZE = DirectX::SimpleMath::Vector2(800);
        nvrhi::Format DS_FORMAT = nvrhi::Format::D32;

        std::vector<DirectX::SimpleMath::Matrix> shadow_views;
        nvrhi::TextureHandle shadow_map_;
        nvrhi::FramebufferHandle shadow_framebuffer_;
    };
}
