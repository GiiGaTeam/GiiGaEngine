#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#include "GBuffer.h"
#include "Material.h"
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

namespace GiiGa
{
    class ForwardPass : public RenderPass
    {
    public:
        static inline const int ViewDataRootIndex = 0;
        static inline const int ModelDataRootIndex = 1;
        static inline const int MaterialDataRootIndex = 2;

        static inline const int ConstantBufferCount = MaterialDataRootIndex + 1;

        ForwardPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
            D3D12_RASTERIZER_DESC rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.CullMode = D3D12_CULL_MODE_BACK;
            rast_desc.FrontCounterClockwise = true;

            auto sampler_desc = D3D12_STATIC_SAMPLER_DESC{
                .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };

            D3D12_BLEND_DESC blend_translucent_desc = {};

            D3D12_RENDER_TARGET_BLEND_DESC renderTargetDesc = {
                .BlendEnable = TRUE,
                .SrcBlend = D3D12_BLEND_SRC_ALPHA,
                .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOp = D3D12_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA,
                .DestBlendAlpha = D3D12_BLEND_ONE,
                .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
            };

            blend_translucent_desc.RenderTarget[0] = renderTargetDesc;

            DXGI_FORMAT rtv_format_array[] = {GBuffer::G_FORMAT};

            mask_to_pso[filter_unlit_solid_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(OpaqueUnlitShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_rtv_format(rtv_format_array, 1)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                    const auto& descs = resource.GetDescriptors();
                    context.BindDescriptorHandle(MaterialDataRootIndex, descs[0]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::EmissiveColor), descs[1]);
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, MaxTextureCount);

            mask_to_pso[filter_translucent_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(OpaqueUnlitShader))
                .set_rasterizer_state(rast_desc)
                .set_blend_state(blend_translucent_desc)
                .set_rtv_format(rtv_format_array, 1)
                .set_input_layout(VertexPNTBT::InputLayout)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                    const auto& descs = resource.GetDescriptors();
                    context.BindDescriptorHandle(MaterialDataRootIndex, descs[0]);
                    context.BindDescriptorHandle(MaterialDataRootIndex + static_cast<int>(TexturesOrder::EmissiveColor), descs[1]);
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, MaxTextureCount);
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

            context.SetSignature(mask_to_pso.begin()->second.GetSignature().get());
            context.BindDescriptorHandle(0, cam_info.viewDescriptor);

            //if (visibles.size() > 0) el::Loggers::getLogger(LogRendering)->debug("some rendering visibles %v", visibles.size());

            for (auto& visible : visibles)
            {
                PSO pso;
                if (!GetPsoFromMapByMask(mask_to_pso, visible.first, pso)) continue;

                context.BindPSO(pso.GetState().get());
                context.SetSignature(pso.GetSignature().get());
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
    };
}
