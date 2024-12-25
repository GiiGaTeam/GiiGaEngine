module;

#include<directxtk12/SimpleMath.h>
#include<d3d12.h>
#include<directx/d3dx12_core.h>

export module GLightPass;

import <memory>;
import <vector>;
import <functional>;

import RenderPass;
import PSO;
import RenderDevice;
import IRenderable;
import SceneVisibility;
import RenderPassViewData;
import Logger;
import ShaderManager;
import RenderContext;
import VertexTypes;
import CameraComponent;
import PerObjectData;
import IObjectShaderResource;
import GPULocalResource;
import RenderSystemSettings;
import GBuffer;
import Material;

namespace GiiGa
{
    export class GLightPass : public RenderPass
    {
        static inline int ViewDataRootIndex = 0;
        static inline int ModelDataRootIndex = 1;
        static inline int LightDataRootIndex = 2;

        static inline int ConstantBufferCount = LightDataRootIndex + 1;

    public:
        GLightPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
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

            DXGI_FORMAT g_format_array[] = {GBuffer::G_FORMAT};

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


            mask_to_pso[ObjectMask()
                        .SetVertexType(VertexTypes::VertexPNTBT)
                        .SetLightType(LightType::Point)]

                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(GPointLight))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_rtv_format(g_format_array, 1)
                .set_dsv_format(GBuffer::DS_FORMAT_DSV)
                .set_depth_stencil_state(depth_stencil_desc)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                    const auto& descs = resource.GetDescriptors();
                    context.BindDescriptorHandle(LightDataRootIndex, descs[0]);
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, GBuffer::NUM_GBUFFERS - 1);
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();

            const auto& visibles = SceneVisibility::Extract(renderpass_filter, renderpass_unite, cam_info.ViewProjMat);

            context.SetSignature(mask_to_pso.begin()->second.GetSignature().get());
            context.BindDescriptorHandle(ViewDataRootIndex, cam_info.viewDescriptor);

            auto accum = gbuffer_->GetRTV(GBuffer::GBufferOrder::LightAccumulation);
            auto depth = gbuffer_->GetDepthRTV();

            context.GetGraphicsCommandList()->OMSetRenderTargets(1, &accum,
                                                                 false, &depth);

            context.BindDescriptorHandle(ConstantBufferCount + 0, gbuffer_->GetSRV(GBuffer::GBufferOrder::Diffuse));
            context.BindDescriptorHandle(ConstantBufferCount + 1, gbuffer_->GetSRV(GBuffer::GBufferOrder::Material));
            context.BindDescriptorHandle(ConstantBufferCount + 2, gbuffer_->GetSRV(GBuffer::GBufferOrder::NormalWS));
            context.BindDescriptorHandle(ConstantBufferCount + 3, gbuffer_->GetSRV(GBuffer::GBufferOrder::PositionWS));

            for (auto& visible : visibles)
            {
                PSO& pso = mask_to_pso.at(visible.first);
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
        ObjectMask renderpass_filter = ObjectMask().SetLightType(LightType::All);

        ObjectMask renderpass_unite = ObjectMask().SetVertexType(VertexTypes::All)
                                                  .SetLightType(LightType::All);

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
