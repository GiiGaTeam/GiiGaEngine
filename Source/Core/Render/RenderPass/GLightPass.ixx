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
            DXGI_FORMAT g_format_array[] = {GBuffer::G_FORMAT};

            auto sampler_desc = D3D12_STATIC_SAMPLER_DESC{
                .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };

            //unmark PSOs
            {
                D3D12_RASTERIZER_DESC unmark_rs = CD3DX12_RASTERIZER_DESC(
                    D3D12_FILL_MODE_SOLID,
                    D3D12_CULL_MODE_BACK,
                    TRUE,
                    D3D12_DEFAULT_DEPTH_BIAS,
                    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
                    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
                    TRUE,
                    FALSE,
                    FALSE,
                    0,
                    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
                );

                D3D12_DEPTH_STENCIL_DESC unmark_ds
                {
                    .DepthEnable = TRUE,
                    .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
                    .DepthFunc = D3D12_COMPARISON_FUNC_GREATER,
                    .StencilEnable = TRUE,
                    .StencilReadMask = 0xff,
                    .StencilWriteMask = 0xff,
                    .FrontFace = {
                        .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilPassOp = D3D12_STENCIL_OP_DECR_SAT,
                        .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
                    },
                    .BackFace = {
                        .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilPassOp = D3D12_STENCIL_OP_DECR_SAT,
                        .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
                    }
                };

                unmark_pso
                    .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                    .set_input_layout(VertexPNTBT::InputLayout)
                    .set_rtv_format(g_format_array, 1)
                    .set_dsv_format(GBuffer::DS_FORMAT_DSV)
                    .set_depth_stencil_state(unmark_ds)
                    .set_rasterizer_state(unmark_rs)
                    .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                    {
                        context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                    })
                    .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                    {
                    })
                    .add_static_samplers(sampler_desc)
                    .GeneratePSO(context.GetDevice(), ConstantBufferCount, GBuffer::NUM_GBUFFERS - 1);
            }

            // shade PSOs
            {
                D3D12_RASTERIZER_DESC shade_rs = {
                    .FillMode = D3D12_FILL_MODE_SOLID,
                    .CullMode = D3D12_CULL_MODE_FRONT,
                    .FrontCounterClockwise = TRUE,
                    .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
                    .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
                    .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
                    .DepthClipEnable = FALSE,
                    .MultisampleEnable = FALSE,
                    .AntialiasedLineEnable = FALSE,
                    .ForcedSampleCount = 0,
                    .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
                };

                D3D12_DEPTH_STENCIL_DESC shade_ds = {
                    .DepthEnable = TRUE,
                    .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
                    .DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                    .StencilEnable = TRUE,
                    .StencilReadMask = 0xff,
                    .StencilWriteMask = 0xff,
                    .FrontFace = D3D12_DEPTH_STENCILOP_DESC{
                        .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                        .StencilFunc = D3D12_COMPARISON_FUNC_EQUAL
                    },
                    .BackFace = {
                        .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                        .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                        .StencilFunc = D3D12_COMPARISON_FUNC_EQUAL
                    },
                };

                D3D12_BLEND_DESC blendDesc = {};
                blendDesc.AlphaToCoverageEnable = FALSE;
                blendDesc.IndependentBlendEnable = FALSE;

                D3D12_RENDER_TARGET_BLEND_DESC renderTargetDesc = {
                    .BlendEnable = TRUE,
                    .SrcBlend = D3D12_BLEND_ONE,
                    .DestBlend = D3D12_BLEND_ONE,
                    .BlendOp = D3D12_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D12_BLEND_ONE,
                    .DestBlendAlpha = D3D12_BLEND_ZERO,
                    .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
                };

                blendDesc.RenderTarget[0] = renderTargetDesc;


                shade_mask_to_pso[ObjectMask()
                                  .SetVertexType(VertexTypes::VertexPNTBT)
                                  .SetLightType(LightType::Point)]

                    .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                    .set_ps(ShaderManager::GetShaderByName(GPointLight))
                    .set_rasterizer_state(shade_rs)
                    .set_input_layout(VertexPNTBT::InputLayout)
                    .set_rtv_format(g_format_array, 1)
                    .set_dsv_format(GBuffer::DS_FORMAT_DSV)
                    .set_depth_stencil_state(shade_ds)
                    .set_blend_state(blendDesc)
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
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();

            const auto& visibles = SceneVisibility::Extract(renderpass_filter, renderpass_unite, cam_info.ViewProjMat);

            context.SetSignature(shade_mask_to_pso.begin()->second.GetSignature().get());
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
                PSO& pso = shade_mask_to_pso.at(visible.first);
                context.SetSignature(pso.GetSignature().get());
                for (auto& common_resource_group : visible.second.common_resource_renderables)
                {
                    for (auto& renderable : common_resource_group.second.renderables)
                    {
                        gbuffer_->ClearStencil(context, 1);
                        pso.SetShaderResources(context, *common_resource_group.second.shaderResource);
                        pso.SetPerObjectData(context, renderable.lock()->GetPerObjectData());

                        {
                            // unmar
                            context.BindPSO(unmark_pso.GetState().get());
                            context.GetGraphicsCommandList()->OMSetStencilRef(1);
                            renderable.lock()->Draw(context);
                        }

                        {
                            // shade
                            PSO& shade_pso = shade_mask_to_pso.at(visible.first);
                            context.BindPSO(shade_pso.GetState().get());
                            context.GetGraphicsCommandList()->OMSetStencilRef(1);
                            renderable.lock()->Draw(context);
                        }
                    }
                }
            }
        }

    private:
        ObjectMask renderpass_filter = ObjectMask().SetLightType(LightType::All);

        ObjectMask renderpass_unite = ObjectMask().SetVertexType(VertexTypes::All)
                                                  .SetLightType(LightType::All);

        std::unordered_map<ObjectMask, PSO> shade_mask_to_pso;
        PSO unmark_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
