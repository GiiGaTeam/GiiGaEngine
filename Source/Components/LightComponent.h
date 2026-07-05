#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directxtk12/SimpleMath.h>

#include <memory>
#include <vector>

#include <nvrhi/nvrhi.h>

#include<Component.h>
#include<IRenderable.h>
#include<TransformComponent.h>
#include<IUpdateGPUData.h>

namespace GiiGa
{
    // NVRHI cutover: the base no longer carries D3D12 shadow plumbing (depth-stencil resource,
    // DSV/SRV descriptor views, viewport/scissor, manual resource transitions). Shadow maps are
    // now NVRHI textures + framebuffers owned by the concrete light that supports them
    // (DirectionalLightComponent). The base only retains the cascade view-projection list used
    // by ShadowPass for frustum culling and the shared transform handle.
    class LightComponent : public Component, public IRenderable, public IUpdateGPUData
    {
    public:
        LightComponent() = default;

        LightComponent(const Json::Value& json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        // Create light-specific shadow resources (NVRHI). Default: no shadows. Directional lights
        // override this to build their cascade depth array + framebuffer + cascade data buffer.
        virtual void InitShadowData(nvrhi::IDevice* /*device*/) {}

        // Per-cascade view-projection matrices (directional only) used by ShadowPass for frustum
        // culling of shadow casters. Point lights return an empty list.
        virtual std::vector<DirectX::SimpleMath::Matrix> GetViews()
        {
            return shadow_views;
        }

    protected:
        std::weak_ptr<TransformComponent> transform_;
        std::vector<DirectX::SimpleMath::Matrix> shadow_views;
    };
}
