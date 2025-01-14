module;
#include<d3d12.h>
#include<directx/d3dx12_core.h>

export module ShadowPass;

import <memory>;
import <vector>;
import <functional>;

import RenderPass;
import Viewport;
import PSO;
import ShaderManager;
import VertexTypes;
import PerObjectData;
import IObjectShaderResource;
import RenderDevice;
import SceneVisibility;
import IRenderable;
import LightComponent;
import DirectionalLightComponent;

namespace GiiGa
{
    export class ShadowPass : public RenderPass
    {
    public:
        static const uint8_t ModelDataRootIndex = 0;
        static const uint8_t DirectionalDataRootIndex = 1;
        static const uint8_t StructuredBufferShadow = 0;
        
        static const uint8_t ConstantBufferCount = 2;
        static const uint8_t SRVCount = 1;

        ShadowPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
            D3D12_RASTERIZER_DESC rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.CullMode = D3D12_CULL_MODE_BACK;
            rast_desc.FrontCounterClockwise = TRUE;

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

            auto sampler_desc = D3D12_STATIC_SAMPLER_DESC{
                .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
                .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };

            mask_to_pso[ObjectMask().
                        SetVertexType(VertexTypes::VertexPNTBT)
                        .SetShadingModel(ShadingModel::DefaultLit)
                        .SetBlendMode(BlendMode::Opaque)
                        .SetFillMode(FillMode::Solid)]
                .set_vs(ShaderManager::GetShaderByName(CascadeShadowShader))
                .set_gs(ShaderManager::GetShaderByName(CascadeShadowGeomShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_dsv_format(DXGI_FORMAT_D24_UNORM_S8_UINT)
                .set_depth_stencil_state(depth_stencil_desc)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, SRVCount);
        }

        void Draw(RenderContext& context) override
        {
            /*auto cam_info = getCamInfoDataFunction_();

            context.SetSignature(mask_to_pso.begin()->second.GetSignature().get());

            std::unordered_map<ObjectMask, DrawPacket> lights;
            SceneVisibility::Extract(lights_filter, renderpass_unite, lights);

            for (const auto& [mask, lightPacket] : lights)
            {
                for (const auto lightGroup : lightPacket.common_resource_renderables)
                {
                    for (const auto rendLight : lightGroup.second.renderables)
                    {
                        // Should be only Directional light
                        const auto light = std::dynamic_pointer_cast<LightComponent>(rendLight.lock());
                        if (!light) continue;

                        if (const auto dirLight = std::dynamic_pointer_cast<DirectionalLightComponent>(light))
                        {
                            dirLight->UpdateCascadeGPUData(context, cam_info.camera);
                            context.BindDescriptorHandle(StructuredBufferShadow, dirLight->GetCascadeDataSRV());
                            context.BindDescriptorHandle(DirectionalDataRootIndex, dirLight->GetLightDataSRV());
                        }

                        const auto lightViews = light->GetViews();
                        std::unordered_map<ObjectMask, DrawPacket> visibles;

                        for (const auto view : lightViews)
                        {
                            SceneVisibility::Expand(objects_filter, renderpass_unite, view, visibles);
                        }

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
                }
            }*/
aaaa        }

    private:
        ObjectMask objects_filter = ObjectMask().SetBlendMode(BlendMode::Opaque)
                                                .SetFillMode(FillMode::Solid)
                                                .SetShadingModel(ShadingModel::DefaultLit);
        ObjectMask lights_filter = ObjectMask().SetLightType(LightType::Directional)
                                               .SetShadingModel(ShadingModel::DefaultLit);

        ObjectMask renderpass_unite = ObjectMask().SetBlendMode(BlendMode::Opaque | BlendMode::Masked)
                                                  .SetShadingModel(ShadingModel::All)
                                                  .SetVertexType(VertexTypes::All)
                                                  .SetFillMode(FillMode::All);


        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
    };
}
