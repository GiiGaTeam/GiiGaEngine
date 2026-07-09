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
#include<DirectionalLightComponent.h>

namespace GiiGa
{
    // Deferred lighting via stencil light-volumes. For point lights a two-phase algorithm runs:
    // an "unmark" pass decrements the stencil where the volume back-faces fail the (reversed)
    // depth test, then an additive "shade" pass lights only the marked pixels. Directional lights
    // shade a full-screen quad and sample the cascade shadow map.
    class GLightPass : public RenderPass
    {
    public:
        GLightPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            auto& device = context.GetDevice();

            sampler_ = device.Device()->createSampler(
                nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(nvrhi::SamplerAddressMode::Wrap));

            comparison_sampler_ = device.Device()->createSampler(
                nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(nvrhi::SamplerAddressMode::Border)
                                    .setReductionType(nvrhi::SamplerReductionType::Comparison));

            // Binding layouts ---------------------------------------------------
            // Unmark: only the view (b0) and per-object (b1) CBs (VertexPNTBT VS, no PS).
            {
                nvrhi::BindingLayoutDesc l;
                l.setVisibility(nvrhi::ShaderType::All);
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
                unmark_layout_ = device.Device()->createBindingLayout(l);
            }
            // Point shade: b0..b3 + Diffuse/Material/Normal (t0..t2) + sampler s0.
            {
                nvrhi::BindingLayoutDesc l;
                l.setVisibility(nvrhi::ShaderType::All);
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(3));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(1));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(2));
                l.addItem(nvrhi::BindingLayoutItem::Sampler(0));
                point_layout_ = device.Device()->createBindingLayout(l);
            }
            // Directional shade: point layout + cascade shadow map (t3), cascade data (t4),
            // comparison sampler (s1).
            {
                nvrhi::BindingLayoutDesc l;
                l.setVisibility(nvrhi::ShaderType::All);
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2));
                l.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(3));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(1));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(2));
                l.addItem(nvrhi::BindingLayoutItem::Texture_SRV(3));
                l.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(4));
                l.addItem(nvrhi::BindingLayoutItem::Sampler(0));
                l.addItem(nvrhi::BindingLayoutItem::Sampler(1));
                dir_layout_ = device.Device()->createBindingLayout(l);
            }

            using SOD = nvrhi::DepthStencilState::StencilOpDesc;

            // Unmark: reversed depth test, decrement stencil on pass, no colour/depth writes.
            {
                nvrhi::DepthStencilState ds;
                ds.enableDepthTest().disableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Greater)
                  .enableStencil().setStencilReadMask(0xff).setStencilWriteMask(0xff)
                  .setFrontFaceStencil(SOD().setPassOp(nvrhi::StencilOp::DecrementAndClamp).setStencilFunc(nvrhi::ComparisonFunc::Always))
                  .setBackFaceStencil(SOD().setPassOp(nvrhi::StencilOp::DecrementAndClamp).setStencilFunc(nvrhi::ComparisonFunc::Always));

                Pipeline p;
                p.VS(ShaderManager::GetShaderByName(device, VertexPNTBTShader))
                 .InputLayout(VertexPNTBT::CreateInputLayout(device, ShaderManager::GetShaderByName(device, VertexPNTBTShader)))
                 .Raster(DefaultRasterState().setCullBack().setFrontCounterClockwise(true))
                 .Depth(ds)
                 .AddBindingLayout(unmark_layout_);
                unmark_pipeline_ = p.Build(device, gbuffer_->GetDepthOnlyFramebuffer());
            }

            nvrhi::BlendState addBlend;
            addBlend.targets[0].enableBlend()
                    .setSrcBlend(nvrhi::BlendFactor::One).setDestBlend(nvrhi::BlendFactor::One).setBlendOp(nvrhi::BlendOp::Add)
                    .setSrcBlendAlpha(nvrhi::BlendFactor::One).setDestBlendAlpha(nvrhi::BlendFactor::Zero).setBlendOpAlpha(nvrhi::BlendOp::Add);

            SOD stencilEqual = SOD().setStencilFunc(nvrhi::ComparisonFunc::Equal);

            // Point shade: front-face cull, reversed depth compare, stencil==1, additive.
            {
                nvrhi::DepthStencilState ds;
                ds.enableDepthTest().disableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::GreaterOrEqual)
                  .enableStencil().setStencilReadMask(0xff).setStencilWriteMask(0xff).setStencilRefValue(1)
                  .setFrontFaceStencil(stencilEqual).setBackFaceStencil(stencilEqual);

                auto vs = ShaderManager::GetShaderByName(device, VertexPNTBTShader);
                Pipeline p;
                p.VS(vs).PS(ShaderManager::GetShaderByName(device, GPointLight))
                 .InputLayout(VertexPNTBT::CreateInputLayout(device, vs))
                 .Raster(DefaultRasterState().setCullFront().setFrontCounterClockwise(true).setDepthClipEnable(false))
                 .Depth(ds).Blend(addBlend)
                 .AddBindingLayout(point_layout_);
                point_shade_ = MaskPipeline{p.Build(device, gbuffer_->GetLightFramebuffer()), point_layout_};
            }

            // Directional shade: full-screen quad, standard depth compare, stencil==1, additive.
            {
                nvrhi::DepthStencilState ds;
                ds.enableDepthTest().disableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Less)
                  .enableStencil().setStencilReadMask(0xff).setStencilWriteMask(0xff).setStencilRefValue(1)
                  .setFrontFaceStencil(stencilEqual).setBackFaceStencil(stencilEqual);

                auto vs = ShaderManager::GetShaderByName(device, VertexFullQuadShader);
                Pipeline p;
                p.VS(vs).PS(ShaderManager::GetShaderByName(device, GDirectionLight))
                 .InputLayout(VertexPNTBT::CreateInputLayout(device, vs))
                 .Raster(DefaultRasterState().setCullFront().setFrontCounterClockwise(true).setDepthClipEnable(false))
                 .Depth(ds).Blend(addBlend)
                 .AddBindingLayout(dir_layout_);
                dir_shade_ = MaskPipeline{p.Build(device, gbuffer_->GetLightFramebuffer()), dir_layout_};
            }
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();
            auto& device = context.GetDevice();

            auto visibles = SceneVisibility::ExtractFromFrustum(filter_pointLight_, cam_info.camera.GetViewProj());
            // Frustum culling should not apply to directional lighting.
            SceneVisibility::ExpandByFilterFromAll(filter_directionalLight_, visibles);

            nvrhi::ViewportState vp;
            vp.addViewportAndScissorRect(nvrhi::Viewport(cam_info.screenDimensions.screenDimensions.x,
                                                         cam_info.screenDimensions.screenDimensions.y));

            for (auto& visible : visibles)
            {
                const bool isDir = filter_directionalLight_.CoversMask(visible.first);
                const MaskPipeline& shade = isDir ? dir_shade_ : point_shade_;

                for (auto& [key, group] : visible.second.common_resource_renderables)
                {
                    for (auto& weak : group.renderables)
                    {
                        auto renderable = weak.lock();
                        if (!renderable) continue;

                        gbuffer_->ClearStencil(context, 1);

                        // Unmark (point lights only).
                        if (!isDir)
                        {
                            nvrhi::BindingSetDesc ub;
                            ub.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewCB));
                            ub.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, renderable->GetPerObjectData().GetBuffer()));
                            nvrhi::BindingSetHandle uset = device.Device()->createBindingSet(ub, unmark_layout_);

                            nvrhi::GraphicsState ustate;
                            ustate.setPipeline(unmark_pipeline_).setFramebuffer(gbuffer_->GetDepthOnlyFramebuffer()).setViewport(vp);
                            ustate.addBindingSet(uset);
                            renderable->Draw(context, ustate);
                        }

                        // Shade.
                        nvrhi::BindingSetDesc bsd;
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewCB));
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, renderable->GetPerObjectData().GetBuffer()));
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, cam_info.dimensionsCB));
                        group.shaderResource->AddBindings(bsd); // light data at b3
                        bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gbuffer_->GetColorTexture(GBuffer::GBufferOrder::Diffuse)));
                        bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(1, gbuffer_->GetColorTexture(GBuffer::GBufferOrder::Material)));
                        bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(2, gbuffer_->GetColorTexture(GBuffer::GBufferOrder::NormalWS)));
                        bsd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));

                        nvrhi::BindingLayoutHandle layout = point_layout_;
                        if (isDir)
                        {
                            auto dir = std::dynamic_pointer_cast<DirectionalLightComponent>(renderable);
                            if (!dir) continue;
                            bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(3, dir->GetShadowTexture()));
                            bsd.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(4, dir->GetCascadeBuffer()));
                            bsd.addItem(nvrhi::BindingSetItem::Sampler(1, comparison_sampler_));
                            layout = dir_layout_;
                        }

                        nvrhi::BindingSetHandle set = device.Device()->createBindingSet(bsd, layout);

                        nvrhi::GraphicsState state;
                        state.setPipeline(shade.pipeline).setFramebuffer(gbuffer_->GetLightFramebuffer()).setViewport(vp);
                        state.addBindingSet(set);
                        renderable->Draw(context, state);
                    }
                }
            }
        }

    private:
        ObjectMask filter_pointLight_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                    .SetLightType(LightType::Point)
                                                    .SetBlendMode(BlendMode::Debug);

        ObjectMask filter_directionalLight_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                          .SetLightType(LightType::Directional)
                                                          .SetBlendMode(BlendMode::Debug);

        nvrhi::BindingLayoutHandle unmark_layout_;
        nvrhi::BindingLayoutHandle point_layout_;
        nvrhi::BindingLayoutHandle dir_layout_;
        nvrhi::GraphicsPipelineHandle unmark_pipeline_;
        MaskPipeline point_shade_;
        MaskPipeline dir_shade_;
        nvrhi::SamplerHandle sampler_;
        nvrhi::SamplerHandle comparison_sampler_;

        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
