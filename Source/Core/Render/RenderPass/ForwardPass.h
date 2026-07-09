#pragma once

#include<memory>
#include<functional>
#include<unordered_map>

#include<nvrhi/nvrhi.h>

#include<RenderPass.h>
#include<Pipeline.h>
#include<IRenderable.h>
#include<SceneVisibility.h>
#include<RenderPassViewData.h>
#include<ShaderManager.h>
#include<RenderContext.h>
#include<VertexTypes.h>
#include<CameraComponent.h>
#include<PerObjectData.h>
#include<IObjectShaderResource.h>
#include<GBuffer.h>
#include<Material.h>

namespace GiiGa
{
    // Forward pass for unlit-opaque and translucent/masked geometry, accumulating into
    // LightAccumulation. Uses the full material binding layout (b0/b1/b2 + t0..t7 + s0) since
    // the OpaqueUnlit shader includes MaterialHeader.
    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            auto& device = context.GetDevice();

            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.setVisibility(nvrhi::ShaderType::All);
            layoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            layoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
            layoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(2));
            for (uint32_t t = 0; t < MaxTextureCount; ++t)
                layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(t));
            bindingLayout_ = device.Device()->createBindingLayout(layoutDesc);

            // Static samplers live in a separate binding set built once (see GBufferPass) to avoid
            // exhausting the 2048-entry shader-visible sampler heap with per-draw allocations.
            nvrhi::BindingLayoutDesc samplerLayoutDesc;
            samplerLayoutDesc.setVisibility(nvrhi::ShaderType::All);
            samplerLayoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            samplerLayout_ = device.Device()->createBindingLayout(samplerLayoutDesc);

            sampler_ = device.Device()->createSampler(
                nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(nvrhi::SamplerAddressMode::Wrap));

            nvrhi::BindingSetDesc samplerSetDesc;
            samplerSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            samplerSet_ = device.Device()->createBindingSet(samplerSetDesc, samplerLayout_);

            auto vs = ShaderManager::GetShaderByName(device, VertexPNTBTShader);
            auto ps = ShaderManager::GetShaderByName(device, OpaqueUnlitShader);
            auto inputLayout = VertexPNTBT::CreateInputLayout(device, vs);

            // Unlit solid (no blend)
            {
                Pipeline p;
                p.VS(vs).PS(ps).InputLayout(inputLayout)
                 .Raster(DefaultRasterState().setCullBack().setFrontCounterClockwise(true))
                 .Depth(DefaultDepthState())
                 .AddBindingLayout(bindingLayout_)
                 .AddBindingLayout(samplerLayout_);
                mask_to_pipeline_[filter_unlit_solid_] = MaskPipeline{p.Build(device, gbuffer_->GetLightFramebuffer()), bindingLayout_};
            }

            // Translucent (alpha blend)
            {
                nvrhi::BlendState blend;
                blend.targets[0].setBlendEnable(true)
                     .setSrcBlend(nvrhi::BlendFactor::SrcAlpha).setDestBlend(nvrhi::BlendFactor::OneMinusSrcAlpha).setBlendOp(nvrhi::BlendOp::Add)
                     .setSrcBlendAlpha(nvrhi::BlendFactor::OneMinusDstAlpha).setDestBlendAlpha(nvrhi::BlendFactor::One).setBlendOpAlpha(nvrhi::BlendOp::Add);

                Pipeline p;
                p.VS(vs).PS(ps).InputLayout(inputLayout)
                 .Raster(DefaultRasterState().setCullBack().setFrontCounterClockwise(true))
                 .Depth(DefaultDepthState())
                 .Blend(blend)
                 .AddBindingLayout(bindingLayout_)
                 .AddBindingLayout(samplerLayout_);
                mask_to_pipeline_[filter_translucent_] = MaskPipeline{p.Build(device, gbuffer_->GetLightFramebuffer()), bindingLayout_};
            }
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();
            auto& device = context.GetDevice();

            const auto& frustum_culling_res = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());
            const auto& vis_unlit = SceneVisibility::Extract(filter_unlit_solid_, frustum_culling_res);
            const auto& vis_translucent = SceneVisibility::Extract(filter_translucent_, frustum_culling_res);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_unlit, visibles);
            SceneVisibility::Expand(vis_translucent, visibles);

            nvrhi::ViewportState vp;
            vp.addViewportAndScissorRect(nvrhi::Viewport(cam_info.screenDimensions.screenDimensions.x,
                                                         cam_info.screenDimensions.screenDimensions.y));

            for (auto& visible : visibles)
            {
                MaskPipeline mp;
                if (!GetPipelineFromMapByMask(mask_to_pipeline_, visible.first, mp)) continue;

                for (auto& [key, group] : visible.second.common_resource_renderables)
                {
                    for (auto& weak : group.renderables)
                    {
                        auto renderable = weak.lock();
                        if (!renderable) continue;

                        nvrhi::BindingSetDesc bsd;
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewCB));
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, renderable->GetPerObjectData().GetBuffer()));
                        group.shaderResource->AddBindings(bsd);
                        nvrhi::BindingSetHandle set = device.Device()->createBindingSet(bsd, bindingLayout_);

                        nvrhi::GraphicsState state;
                        state.setPipeline(mp.pipeline).setFramebuffer(gbuffer_->GetLightFramebuffer()).setViewport(vp);
                        state.addBindingSet(set).addBindingSet(samplerSet_);

                        renderable->Draw(context, state);
                    }
                }
            }
        }

    private:
        ObjectMask filter_unlit_solid_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                     .SetShadingModel(ShadingModel::Unlit)
                                                     .SetBlendMode(BlendMode::Opaque)
                                                     .SetFillMode(FillMode::Solid);

        ObjectMask filter_translucent_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                     .SetShadingModel(ShadingModel::All)
                                                     .SetBlendMode(BlendMode::Translucent | BlendMode::Masked)
                                                     .SetFillMode(FillMode::All);

        std::unordered_map<ObjectMask, MaskPipeline> mask_to_pipeline_;
        nvrhi::BindingLayoutHandle bindingLayout_;
        nvrhi::BindingLayoutHandle samplerLayout_;
        nvrhi::SamplerHandle sampler_;
        nvrhi::BindingSetHandle samplerSet_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
