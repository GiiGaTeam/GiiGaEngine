#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>


#include<memory>
#include<vector>
#include<functional>
#include<tracy/tracy/Tracy.hpp>
#include<TracyGpu.h>

#include<RenderPass.h>
#include<PSO.h>
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
    // Deferred lighting with the two-phase stencil-volume algorithm:
    // an "unmark" pass decrements stencil for covered light volumes, then the
    // shade pass (additive blending, stencil ref = 1) accumulates lighting into
    // LightAccumulation. The GBuffer depth is sampled through a copy since the
    // real depth buffer stays bound for depth/stencil testing.
    class GLightPass : public RenderPass
    {
    public:
        GLightPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            nvrhi::SamplerDesc sampler_desc;
            sampler_desc.minFilter = sampler_desc.magFilter = sampler_desc.mipFilter = true;
            sampler_desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);

            nvrhi::SamplerDesc compression_sampler_desc;
            compression_sampler_desc.minFilter = compression_sampler_desc.magFilter = true;
            compression_sampler_desc.mipFilter = false;
            compression_sampler_desc.setAllAddressModes(nvrhi::SamplerAddressMode::Border);
            compression_sampler_desc.borderColor = nvrhi::Color(0.0f);
            compression_sampler_desc.reductionType = nvrhi::SamplerReductionType::Comparison;

            //unmark PSO
            {
                nvrhi::RasterState unmark_rs;
                unmark_rs.fillMode = nvrhi::RasterFillMode::Solid;
                unmark_rs.cullMode = nvrhi::RasterCullMode::Back;
                unmark_rs.frontCounterClockwise = true;
                unmark_rs.depthClipEnable = true;

                nvrhi::DepthStencilState unmark_ds;
                unmark_ds.depthTestEnable = true;
                unmark_ds.depthWriteEnable = false;
                unmark_ds.depthFunc = nvrhi::ComparisonFunc::Greater;
                unmark_ds.stencilEnable = true;
                unmark_ds.stencilReadMask = 0xff;
                unmark_ds.stencilWriteMask = 0xff;
                unmark_ds.frontFaceStencil.failOp = nvrhi::StencilOp::Keep;
                unmark_ds.frontFaceStencil.depthFailOp = nvrhi::StencilOp::Keep;
                unmark_ds.frontFaceStencil.passOp = nvrhi::StencilOp::DecrementAndClamp;
                unmark_ds.frontFaceStencil.stencilFunc = nvrhi::ComparisonFunc::Always;
                unmark_ds.backFaceStencil = unmark_ds.frontFaceStencil;

                unmark_pso
                    .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                    .set_input_layout(VertexPNTBT::InputLayout)
                    .set_depth_stencil_state(unmark_ds)
                    .set_rasterizer_state(unmark_rs)
                    .add_constant_buffer(0) // view
                    .add_constant_buffer(1) // per-object
                    .GeneratePSO(context.GetDevice());
            }

            // shade PSOs
            {
                nvrhi::RasterState shade_rs;
                shade_rs.fillMode = nvrhi::RasterFillMode::Solid;
                shade_rs.cullMode = nvrhi::RasterCullMode::Front;
                shade_rs.frontCounterClockwise = true;
                shade_rs.depthClipEnable = false;

                nvrhi::DepthStencilState shade_ds;
                shade_ds.depthTestEnable = true;
                shade_ds.depthWriteEnable = false;
                shade_ds.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                shade_ds.stencilEnable = true;
                shade_ds.stencilReadMask = 0xff;
                shade_ds.stencilWriteMask = 0xff;
                shade_ds.stencilRefValue = 1;
                shade_ds.frontFaceStencil.failOp = nvrhi::StencilOp::Keep;
                shade_ds.frontFaceStencil.depthFailOp = nvrhi::StencilOp::Keep;
                shade_ds.frontFaceStencil.passOp = nvrhi::StencilOp::Keep;
                shade_ds.frontFaceStencil.stencilFunc = nvrhi::ComparisonFunc::Equal;
                shade_ds.backFaceStencil = shade_ds.frontFaceStencil;

                nvrhi::DepthStencilState shade_dir_ds = shade_ds;
                shade_dir_ds.depthFunc = nvrhi::ComparisonFunc::Less;

                nvrhi::BlendState blend_state;
                blend_state.targets[0].blendEnable = true;
                blend_state.targets[0].srcBlend = nvrhi::BlendFactor::One;
                blend_state.targets[0].destBlend = nvrhi::BlendFactor::One;
                blend_state.targets[0].blendOp = nvrhi::BlendOp::Add;
                blend_state.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
                blend_state.targets[0].destBlendAlpha = nvrhi::BlendFactor::Zero;
                blend_state.targets[0].blendOpAlpha = nvrhi::BlendOp::Add;

                shade_mask_to_pso[filter_pointLight_]
                    .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                    .set_ps(ShaderManager::GetShaderByName(GPointLight))
                    .set_rasterizer_state(shade_rs)
                    .set_input_layout(VertexPNTBT::InputLayout)
                    .set_depth_stencil_state(shade_ds)
                    .set_blend_state(blend_state)
                    // registers of GPointLight.hlsl
                    .add_constant_buffer(0) // view
                    .add_constant_buffer(1) // per-object
                    .add_constant_buffer(2) // screen dimensions
                    .add_constant_buffer(3) // light data
                    .add_texture_srv(0)     // Diffuse
                    .add_texture_srv(1)     // MatProp
                    .add_texture_srv(2)     // NormalWS
                    .add_sampler(0, sampler_desc)
                    .GeneratePSO(context.GetDevice());

                shade_mask_to_pso[filter_directionalLight_]
                    .set_vs(ShaderManager::GetShaderByName(VertexFullQuadShader))
                    .set_ps(ShaderManager::GetShaderByName(GDirectionLight))
                    .set_rasterizer_state(shade_rs)
                    .set_input_layout(VertexPNTBT::InputLayout)
                    .set_depth_stencil_state(shade_dir_ds)
                    .set_blend_state(blend_state)
                    // registers of GDirectionalLight.hlsl
                    .add_constant_buffer(0)          // view
                    .add_constant_buffer(1)          // per-object
                    .add_constant_buffer(2)          // screen dimensions
                    .add_constant_buffer(3)          // light data
                    .add_texture_srv(0)              // Diffuse
                    .add_texture_srv(1)              // MatProp
                    .add_texture_srv(2)              // NormalWS
                    .add_texture_srv(3)              // CascadeShadowMaps
                    .add_structured_buffer_srv(4)    // cascadeData
                    .add_sampler(0, sampler_desc)
                    .add_sampler(1, compression_sampler_desc)
                    .GeneratePSO(context.GetDevice());
            }
        }

        void Draw(RenderContext& context) override
        {
            ZoneScoped;
            GIIGA_GPU_ZONE(context, "GLight");
            auto cam_info = getCamInfoDataFunction_();

            auto visibles = SceneVisibility::ExtractFromFrustum(filter_pointLight_, cam_info.camera.GetViewProj());

            // Frustum culling should not work for directional lighting.
            SceneVisibility::ExpandByFilterFromAll(filter_directionalLight_, visibles);

            for (auto& visible : visibles)
            {
                PSO* shade_pso = GetPsoFromMapByMask(shade_mask_to_pso, visible.first);
                if (!shade_pso) continue;

                for (auto& common_resource_group : visible.second.common_resource_renderables)
                {
                    for (auto& renderable : common_resource_group.second.renderables)
                    {
                        gbuffer_->ClearStencil(context, 1);

                        const bool is_directional = filter_directionalLight_.CoversMask(renderable.lock()->GetSortData().object_mask);

                        if (!is_directional)
                        {
                            // unmark
                            nvrhi::BindingSetDesc binding_desc;
                            binding_desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewConstantBuffer));
                            unmark_pso.AppendPerObjectData(binding_desc, renderable.lock()->GetPerObjectData());

                            context.SetFramebuffer(gbuffer_->GetDepthOnlyFramebuffer());
                            context.SetPipeline(unmark_pso.GetOrCreatePipeline(context.GetDevice(), gbuffer_->GetDepthOnlyFramebuffer()));
                            context.SetBindingSet(context.GetOrCreateBindingSet(binding_desc, unmark_pso.GetBindingLayout()));
                            renderable.lock()->Draw(context);
                        }

                        {
                            // shade
                            nvrhi::BindingSetDesc binding_desc;
                            binding_desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewConstantBuffer));
                            binding_desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, cam_info.dimensionsConstantBuffer));
                            binding_desc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gbuffer_->GetTexture(GBuffer::GBufferOrder::Diffuse)));
                            binding_desc.addItem(nvrhi::BindingSetItem::Texture_SRV(1, gbuffer_->GetTexture(GBuffer::GBufferOrder::Material)));
                            binding_desc.addItem(nvrhi::BindingSetItem::Texture_SRV(2, gbuffer_->GetTexture(GBuffer::GBufferOrder::NormalWS)));

                            if (auto dir_light = std::dynamic_pointer_cast<DirectionalLightComponent>(renderable.lock()))
                            {
                                binding_desc.addItem(nvrhi::BindingSetItem::Texture_SRV(3, dir_light->GetShadowMap()));
                                binding_desc.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(4, dir_light->GetCascadeDataBuffer()));
                            }

                            shade_pso->AppendSamplers(binding_desc);
                            shade_pso->AppendShaderResources(binding_desc, *common_resource_group.second.shaderResource);
                            shade_pso->AppendPerObjectData(binding_desc, renderable.lock()->GetPerObjectData());

                            context.SetFramebuffer(gbuffer_->GetAccumFramebuffer());
                            context.SetPipeline(shade_pso->GetOrCreatePipeline(context.GetDevice(), gbuffer_->GetAccumFramebuffer()));
                            context.SetBindingSet(context.GetOrCreateBindingSet(binding_desc, shade_pso->GetBindingLayout()));
                            renderable.lock()->Draw(context);
                        }
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

        std::unordered_map<ObjectMask, PSO> shade_mask_to_pso;
        PSO unmark_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
