#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>


#include<memory>
#include<vector>
#include<functional>

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
#include<Material.h>

namespace GiiGa
{
    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            nvrhi::RasterState rast_state;
            rast_state.fillMode = nvrhi::RasterFillMode::Solid;
            rast_state.cullMode = nvrhi::RasterCullMode::Back;
            rast_state.frontCounterClockwise = true;

            nvrhi::SamplerDesc sampler_desc;
            sampler_desc.minFilter = sampler_desc.magFilter = sampler_desc.mipFilter = true;
            sampler_desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);

            nvrhi::BlendState blend_translucent;
            blend_translucent.targets[0].blendEnable = true;
            blend_translucent.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
            blend_translucent.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
            blend_translucent.targets[0].blendOp = nvrhi::BlendOp::Add;
            blend_translucent.targets[0].srcBlendAlpha = nvrhi::BlendFactor::InvDstAlpha;
            blend_translucent.targets[0].destBlendAlpha = nvrhi::BlendFactor::One;
            blend_translucent.targets[0].blendOpAlpha = nvrhi::BlendOp::Add;

            mask_to_pso[filter_unlit_solid_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(OpaqueUnlitShader))
                .set_rasterizer_state(rast_state)
                .set_input_layout(VertexPNTBT::InputLayout)
                // registers of OpaqueUnlitPixelShader: material CB + emissive texture
                .add_constant_buffer(0)
                .add_constant_buffer(1)
                .add_constant_buffer(2)
                .add_texture_srv(static_cast<int>(TexturesOrder::EmissiveColor) - 1)
                .add_sampler(0, sampler_desc)
                .GeneratePSO(context.GetDevice());

            mask_to_pso[filter_translucent_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(OpaqueUnlitShader))
                .set_rasterizer_state(rast_state)
                .set_blend_state(blend_translucent)
                .set_input_layout(VertexPNTBT::InputLayout)
                .add_constant_buffer(0)
                .add_constant_buffer(1)
                .add_constant_buffer(2)
                .add_texture_srv(static_cast<int>(TexturesOrder::EmissiveColor) - 1)
                .add_sampler(0, sampler_desc)
                .GeneratePSO(context.GetDevice());
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();

            const auto& frustum_culling_res = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());

            const auto& vis_unlit = SceneVisibility::Extract(filter_unlit_solid_, frustum_culling_res);
            const auto& vis_translucent = SceneVisibility::Extract(filter_translucent_, frustum_culling_res);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_unlit, visibles);
            SceneVisibility::Expand(vis_translucent, visibles);

            context.SetFramebuffer(gbuffer_->GetAccumFramebuffer());

            for (auto& visible : visibles)
            {
                PSO* pso = GetPsoFromMapByMask(mask_to_pso, visible.first);
                if (!pso) continue;

                context.SetPipeline(pso->GetOrCreatePipeline(context.GetDevice(), gbuffer_->GetAccumFramebuffer()));

                for (auto& common_resource_group : visible.second.common_resource_renderables)
                {
                    auto CRG = common_resource_group.second;
                    for (auto& renderable : common_resource_group.second.renderables)
                    {
                        nvrhi::BindingSetDesc binding_desc;
                        binding_desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewConstantBuffer));
                        pso->AppendSamplers(binding_desc);
                        pso->AppendShaderResources(binding_desc, *CRG.shaderResource);
                        pso->AppendPerObjectData(binding_desc, renderable.lock()->GetPerObjectData());

                        context.SetBindingSet(context.GetOrCreateBindingSet(binding_desc, pso->GetBindingLayout()));
                        renderable.lock()->Draw(context);
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

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
