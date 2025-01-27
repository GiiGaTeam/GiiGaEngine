#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directx/d3dx12.h>
#include <directxtk12/SimpleMath.h>

#include<memory>

#include<Component.h>
#include<Material.h>
#include<IRenderable.h>
#include<TransformComponent.h>
#include<IUpdateGPUData.h>
#include<RenderDevice.h>
#include<GPULocalResource.h>

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

        virtual void ClearShadowDSV(RenderContext& context)
        {
            context.ClearDepthStencilView(shadow_DSV_->getDescriptor().GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, DS_CLEAR_VALUE);
        }

        virtual void TransitionDepthShadowResource(RenderContext& context, D3D12_RESOURCE_STATES after) const
        {
            shadow_resource_->StateTransition(context, after);
        }

        virtual void BindShadowDSV(RenderContext& context) const
        {
            auto dsv = shadow_DSV_->getDescriptor().GetCpuHandle();
            context.GetGraphicsCommandList()->OMSetRenderTargets(0, nullptr,
                                                                 false, &dsv);
        }

        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShadowDSV() const
        {
            return shadow_DSV_->getDescriptor().GetCpuHandle();
        }

        virtual D3D12_GPU_DESCRIPTOR_HANDLE GetShadowSRV() const
        {
            return shadow_SRV_->getDescriptor().getGPUHandle();
        }

        virtual std::vector<DirectX::SimpleMath::Matrix> GetViews()
        {
            return shadow_views;
        }

    protected:
        std::weak_ptr<TransformComponent> transform_;

        DirectX::SimpleMath::Vector2 TEXTURE_SIZE = DirectX::SimpleMath::Vector2(800);
        DXGI_FORMAT DS_FORMAT_RES = DXGI_FORMAT_R32_TYPELESS;
        DXGI_FORMAT DS_FORMAT_DSV = DXGI_FORMAT_D32_FLOAT;
        DXGI_FORMAT DS_FORMAT_SRV = DXGI_FORMAT_R32_FLOAT;
        D3D12_RESOURCE_DIMENSION DS_DIMENSION_RES = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        D3D12_DSV_DIMENSION DS_DIMENSION_DSV = D3D12_DSV_DIMENSION_TEXTURE2D;
        D3D12_SRV_DIMENSION DS_DIMENSION_SRV = D3D12_SRV_DIMENSION_TEXTURE2D;
        D3D12_CLEAR_VALUE DS_CLEAR_VALUE = {.Format = DS_FORMAT_DSV, .DepthStencil = {.Depth = 1, .Stencil = 1}};

        std::vector<DirectX::SimpleMath::Matrix> shadow_views;
        std::unique_ptr<GPULocalResource> shadow_resource_;
        std::shared_ptr<BufferView<ShaderResource>> shadow_SRV_;
        std::shared_ptr<BufferView<DepthStencil>> shadow_DSV_;
    };
}
