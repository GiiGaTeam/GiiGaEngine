#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<directx/d3dx12_core.h>



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
    class GBufferPass : public RenderPass
    {
    public:
        static inline const int ViewDataRootIndex = 0;
        static inline const int ModelDataRootIndex = 1;
        static inline const int MaterialDataRootIndex = 2;

        static inline const int ConstantBufferCount = MaterialDataRootIndex + 1;

        GBufferPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            D3D12_RASTERIZER_DESC rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.CullMode = D3D12_CULL_MODE_BACK;
            rast_desc.FrontCounterClockwise = TRUE;

            auto sampler_desc = D3D12_STATIC_SAMPLER_DESC{
                .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };

            DXGI_FORMAT g_format_array[] = {GBuffer::G_FORMAT, GBuffer::G_FORMAT, GBuffer::G_FORMAT, GBuffer::G_FORMAT, GBuffer::G_FORMAT};

            D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = {
                .DepthEnable = TRUE,
                .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
                .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
                .StencilEnable = FALSE,
                .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
                .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
                .FrontFace = {
                    .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                    .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                    .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                    .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
                },
                .BackFace = {
                    .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                    .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                    .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                    .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
                }
            };

            mask_to_pso[filter_lit_solid_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(GBufferOpaqueDefaultLitShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_rtv_format(g_format_array, 5)
                .set_dsv_format(GBuffer::DS_FORMAT_DSV)
                .set_depth_stencil_state(depth_stencil_desc)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                    const auto& descs = resource.GetDescriptors();
                    context.BindDescriptorHandle(MaterialDataRootIndex, descs[0]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::BaseColor), descs[1]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::Metallic), descs[2]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::Specular), descs[3]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::Roughness), descs[4]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::Anisotropy), descs[5]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::EmissiveColor), descs[6]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::Normal), descs[7]);
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, MaxTextureCount);

            rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
            rast_desc.CullMode = D3D12_CULL_MODE_NONE;
            rast_desc.FrontCounterClockwise = TRUE;

            mask_to_pso[filter_wire_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(GBufferWireframeShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_rtv_format(g_format_array, 5)
                .set_dsv_format(GBuffer::DS_FORMAT_DSV)
                .set_depth_stencil_state(depth_stencil_desc)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, MaxTextureCount);
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();
            const auto& frustum_culling_res = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());
            const auto& vis_lit = SceneVisibility::Extract(filter_lit_solid_, frustum_culling_res);
            const auto& vis_wire = SceneVisibility::Extract(filter_wire_, frustum_culling_res);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_lit, visibles);
            SceneVisibility::Expand(vis_wire, visibles);

            context.SetSignature(mask_to_pso.begin()->second.GetSignature().get());

            gbuffer_->TransitionResource(context, GBuffer::GBufferOrder::LightAccumulation, D3D12_RESOURCE_STATE_RENDER_TARGET);
            gbuffer_->TransitionResource(context, GBuffer::GBufferOrder::Diffuse, D3D12_RESOURCE_STATE_RENDER_TARGET);
            gbuffer_->TransitionResource(context, GBuffer::GBufferOrder::Material, D3D12_RESOURCE_STATE_RENDER_TARGET);
            gbuffer_->TransitionResource(context, GBuffer::GBufferOrder::NormalWS, D3D12_RESOURCE_STATE_RENDER_TARGET);
            //gbuffer_->TransitionResource(context, GBuffer::GBufferOrder::PositionWS, D3D12_RESOURCE_STATE_RENDER_TARGET);

            gbuffer_->BindAllAsRTV(context);

            gbuffer_->ClearAll(context);
            context.BindDescriptorHandle(ViewDataRootIndex, cam_info.viewDescriptor);

            for (auto& visible : visibles)
            {
                PSO pso;
                if (!GetPsoFromMapByMask(mask_to_pso, visible.first, pso)) continue;

                context.SetSignature(pso.GetSignature().get());
                context.BindPSO(pso.GetState().get());
                for (auto& common_resource_group : visible.second.common_resource_renderables)
                {
                    auto CRG = common_resource_group.second;
                    pso.SetShaderResources(context, *CRG.shaderResource);
                    for (auto& renderable : common_resource_group.second.renderables)
                    {
                        pso.SetPerObjectData(context, renderable.lock()->GetPerObjectData());
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
                                              .SetFillMode(FillMode::Wire)
                                              .SetLightType(LightType::All);

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
