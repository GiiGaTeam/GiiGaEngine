#pragma once

#include<memory>
#include<vector>
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
    class GBufferPass : public RenderPass
    {
    public:
        GBufferPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            auto& device = context.GetDevice();

            // Binding layout matching the GBuffer shaders: b0 view, b1 object, b2 material,
            // t0..t7 material textures, s0 sampler.
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.setVisibility(nvrhi::ShaderType::All);
            layoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            layoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
            layoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(2));
            for (uint32_t t = 0; t < MaxTextureCount; ++t)
                layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(t));
            bindingLayout_ = device.Device()->createBindingLayout(layoutDesc);

            // Samplers are static, so keep them in a separate binding set built once and reused for
            // every draw. Allocating a sampler descriptor per draw would exhaust the shader-visible
            // sampler heap (hard-capped at 2048 entries).
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
            auto inputLayout = VertexPNTBT::CreateInputLayout(device, vs);

            // Lit solid
            {
                Pipeline p;
                p.VS(vs)
                 .PS(ShaderManager::GetShaderByName(device, GBufferOpaqueDefaultLitShader))
                 .InputLayout(inputLayout)
                 .Raster(DefaultRasterState().setCullBack().setFrontCounterClockwise(true))
                 .Depth(DefaultDepthState())
                 .AddBindingLayout(bindingLayout_)
                 .AddBindingLayout(samplerLayout_);
                MaskPipeline mp{p.Build(device, gbuffer_->GetFramebuffer()), bindingLayout_};
                mask_to_pipeline_[filter_lit_solid_] = mp;
            }

            // Wireframe variant
            {
                Pipeline p;
                p.VS(vs)
                 .PS(ShaderManager::GetShaderByName(device, GBufferWireframeShader))
                 .InputLayout(inputLayout)
                 .Raster(DefaultRasterState().setFillWireframe().setCullNone().setFrontCounterClockwise(true))
                 .Depth(DefaultDepthState())
                 .AddBindingLayout(bindingLayout_)
                 .AddBindingLayout(samplerLayout_);
                MaskPipeline mp{p.Build(device, gbuffer_->GetFramebuffer()), bindingLayout_};
                mask_to_pipeline_[filter_wire_] = mp;
            }
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();
            auto* cl = context.GetCommandList();
            auto& device = context.GetDevice();

            const auto& frustum_culling_res = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());
            const auto& vis_lit = SceneVisibility::Extract(filter_lit_solid_, frustum_culling_res);
            const auto& vis_wire = SceneVisibility::Extract(filter_wire_, frustum_culling_res);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_lit, visibles);
            SceneVisibility::Expand(vis_wire, visibles);

            gbuffer_->ClearAll(context);

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
                        state.setPipeline(mp.pipeline).setFramebuffer(gbuffer_->GetFramebuffer()).setViewport(vp);
                        state.addBindingSet(set).addBindingSet(samplerSet_);

                        renderable->Draw(context, state);
                    }
                }
            }
        }

    private:
        ObjectMask filter_lit_solid_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                   .SetShadingModel(ShadingModel::DefaultLit)
                                                   .SetBlendMode(BlendMode::Opaque)
                                                   .SetFillMode(FillMode::Solid);

        ObjectMask filter_wire_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                              .SetShadingModel(ShadingModel::DefaultLit)
                                              .SetBlendMode(BlendMode::Opaque)
                                              .SetFillMode(FillMode::Wire);

        std::unordered_map<ObjectMask, MaskPipeline> mask_to_pipeline_;
        nvrhi::BindingLayoutHandle bindingLayout_;
        nvrhi::BindingLayoutHandle samplerLayout_;
        nvrhi::SamplerHandle sampler_;
        nvrhi::BindingSetHandle samplerSet_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
