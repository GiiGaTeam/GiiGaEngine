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
            nvrhi::RasterState rast_state;
            rast_state.fillMode = nvrhi::RasterFillMode::Solid;
            rast_state.cullMode = nvrhi::RasterCullMode::Back;
            rast_state.frontCounterClockwise = true;

            nvrhi::SamplerDesc sampler_desc;
            sampler_desc.minFilter = sampler_desc.magFilter = sampler_desc.mipFilter = true;
            sampler_desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);

            nvrhi::DepthStencilState depth_stencil_state;
            depth_stencil_state.depthTestEnable = true;
            depth_stencil_state.depthWriteEnable = true;
            depth_stencil_state.depthFunc = nvrhi::ComparisonFunc::Less;
            depth_stencil_state.stencilEnable = false;

            mask_to_pso[filter_lit_solid_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(GBufferOpaqueDefaultLitShader))
                .set_rasterizer_state(rast_state)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_depth_stencil_state(depth_stencil_state)
                // registers used by VertexPNTBTShader + GBufferOpaqueDefaultLitPixelShader
                .add_constant_buffer(0) // view
                .add_constant_buffer(1) // per-object
                .add_constant_buffer(2) // material
                .add_texture_srv(static_cast<int>(TexturesOrder::BaseColor) - 1)
                .add_texture_srv(static_cast<int>(TexturesOrder::Metallic) - 1)
                .add_texture_srv(static_cast<int>(TexturesOrder::Specular) - 1)
                .add_texture_srv(static_cast<int>(TexturesOrder::Roughness) - 1)
                .add_texture_srv(static_cast<int>(TexturesOrder::Anisotropy) - 1)
                .add_texture_srv(static_cast<int>(TexturesOrder::EmissiveColor) - 1)
                .add_texture_srv(static_cast<int>(TexturesOrder::Normal) - 1)
                .add_sampler(0, sampler_desc)
                .GeneratePSO(context.GetDevice());

            nvrhi::RasterState wire_rast_state;
            wire_rast_state.fillMode = nvrhi::RasterFillMode::Wireframe;
            wire_rast_state.cullMode = nvrhi::RasterCullMode::None;
            wire_rast_state.frontCounterClockwise = true;

            mask_to_pso[filter_wire_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(GBufferWireframeShader))
                .set_rasterizer_state(wire_rast_state)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_depth_stencil_state(depth_stencil_state)
                .add_constant_buffer(0)
                .add_constant_buffer(1)
                .GeneratePSO(context.GetDevice());
        }

        void Draw(RenderContext& context) override
        {
            ZoneScoped;
            GIIGA_GPU_ZONE(context, "GBuffer");
            auto cam_info = getCamInfoDataFunction_();
            const auto& frustum_culling_res = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());
            const auto& vis_lit = SceneVisibility::Extract(filter_lit_solid_, frustum_culling_res);
            const auto& vis_wire = SceneVisibility::Extract(filter_wire_, frustum_culling_res);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_lit, visibles);
            SceneVisibility::Expand(vis_wire, visibles);

            context.SetFramebuffer(gbuffer_->GetGeometryFramebuffer());
            gbuffer_->ClearAll(context);

            for (auto& visible : visibles)
            {
                PSO* pso = GetPsoFromMapByMask(mask_to_pso, visible.first);
                if (!pso) continue;

                context.SetPipeline(pso->GetOrCreatePipeline(context.GetDevice(), gbuffer_->GetGeometryFramebuffer()));

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
        ObjectMask filter_lit_solid_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                   .SetShadingModel(ShadingModel::DefaultLit)
                                                   .SetBlendMode(BlendMode::Opaque)
                                                   .SetFillMode(FillMode::Solid);

        ObjectMask filter_wire_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                              .SetShadingModel(ShadingModel::DefaultLit)
                                              .SetBlendMode(BlendMode::Opaque)
                                              .SetFillMode(FillMode::Wire);

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
