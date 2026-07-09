#pragma once

#include<memory>
#include<vector>
#include<functional>
#include<unordered_map>

#include<nvrhi/nvrhi.h>

#include<RenderPass.h>
#include<Pipeline.h>
#include<ShaderManager.h>
#include<VertexTypes.h>
#include<PerObjectData.h>
#include<SceneVisibility.h>
#include<IRenderable.h>
#include<RenderPassViewData.h>
#include<LightComponent.h>
#include<DirectionalLightComponent.h>

namespace GiiGa
{
    // Cascaded shadow-map generation for directional lights. A vertex + geometry shader pair
    // renders shadow-casting geometry into all cascade slices of the light's depth array in one
    // draw (the GS routes each triangle to its cascade via SV_RenderTargetArrayIndex).
    class ShadowPass : public RenderPass
    {
    public:
        ShadowPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
            auto& device = context.GetDevice();

            // b0 per-object world, b1 directional-light data, t0 cascade data (read by the GS).
            nvrhi::BindingLayoutDesc l;
            l.setVisibility(nvrhi::ShaderType::All);
            l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
            l.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0));
            bindingLayout_ = device.Device()->createBindingLayout(l);
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();
            auto& device = context.GetDevice();

            std::unordered_map<ObjectMask, DrawPacket> lights;
            SceneVisibility::ExpandByFilterFromAll(filter_lights_, lights);

            for (const auto& [mask, lightPacket] : lights)
            {
                for (const auto& lightGroup : lightPacket.common_resource_renderables)
                {
                    for (const auto& rendLight : lightGroup.second.renderables)
                    {
                        const auto dirLight = std::dynamic_pointer_cast<DirectionalLightComponent>(rendLight.lock());
                        if (!dirLight) continue;

                        BuildPipelineIfNeeded(device, dirLight->GetShadowFramebuffer());

                        dirLight->UpdateCascadeGPUData(context, cam_info.camera);

                        context.GetCommandList()->clearDepthStencilTexture(
                            dirLight->GetShadowTexture(), nvrhi::AllSubresources, true, 1.0f, false, 0);

                        const auto texSize = dirLight->GetShadowTextureSize();
                        nvrhi::ViewportState vp;
                        vp.addViewportAndScissorRect(nvrhi::Viewport(texSize.x, texSize.y));

                        std::unordered_map<ObjectMask, DrawPacket> visibles;
                        for (const auto& view : dirLight->GetViews())
                            SceneVisibility::ExpandByFilterFromFrustum(filter_objects_, view, visibles);

                        for (auto& visible : visibles)
                        {
                            if (!filter_objects_.CoversMask(visible.first)) continue;

                            for (auto& [key, group] : visible.second.common_resource_renderables)
                            {
                                for (auto& weak : group.renderables)
                                {
                                    auto renderable = weak.lock();
                                    if (!renderable) continue;

                                    nvrhi::BindingSetDesc bsd;
                                    bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, renderable->GetPerObjectData().GetBuffer()));
                                    bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, dirLight->GetLightCB()));
                                    bsd.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(0, dirLight->GetCascadeBuffer()));
                                    nvrhi::BindingSetHandle set = device.Device()->createBindingSet(bsd, bindingLayout_);

                                    nvrhi::GraphicsState state;
                                    state.setPipeline(pipeline_).setFramebuffer(dirLight->GetShadowFramebuffer()).setViewport(vp);
                                    state.addBindingSet(set);

                                    renderable->Draw(context, state);
                                }
                            }
                        }
                    }
                }
            }
        }

    private:
        void BuildPipelineIfNeeded(RenderDevice& device, nvrhi::IFramebuffer* fb)
        {
            if (pipeline_) return;

            auto vs = ShaderManager::GetShaderByName(device, CascadeShadowShader);

            Pipeline p;
            p.VS(vs)
             .GS(ShaderManager::GetShaderByName(device, CascadeShadowGeomShader))
             .InputLayout(VertexPNTBT::CreateInputLayout(device, vs))
             .Raster(DefaultRasterState().setCullBack().setFrontCounterClockwise(true)
                                         .setDepthBias(DepthBias_).setDepthBiasClamp(DepthBiasClamp_).setSlopeScaleDepthBias(SlopeScaledDepthBias_))
             .Depth(DefaultDepthState())
             .AddBindingLayout(bindingLayout_);
            pipeline_ = p.Build(device, fb);
        }

        ObjectMask filter_objects_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                 .SetBlendMode(BlendMode::Opaque | BlendMode::Masked)
                                                 .SetShadingModel(ShadingModel::DefaultLit)
                                                 .SetFillMode(FillMode::Solid);

        ObjectMask filter_lights_ = ObjectMask().SetLightType(LightType::Directional)
                                                .SetShadingModel(ShadingModel::All)
                                                .SetVertexType(VertexTypes::All)
                                                .SetFillMode(FillMode::All);

        nvrhi::BindingLayoutHandle bindingLayout_;
        nvrhi::GraphicsPipelineHandle pipeline_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        int DepthBias_ = -15000;
        float DepthBiasClamp_ = 0;
        float SlopeScaledDepthBias_ = 8.000f;
    };
}
