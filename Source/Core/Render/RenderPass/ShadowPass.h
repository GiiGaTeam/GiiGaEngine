#pragma once

#include<pybind11/pybind11.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<directx/d3dx12_core.h>



#include<memory>
#include<vector>
#include<functional>

#include<RenderPass.h>
#include<Viewport.h>
#include<PSO.h>
#include<ShaderManager.h>
#include<VertexTypes.h>
#include<PerObjectData.h>
#include<SceneVisibility.h>
#include<IRenderable.h>
#include<LightComponent.h>
#include<DirectionalLightComponent.h>

namespace GiiGa
{
    class ShadowPass : public RenderPass
    {
    public:
        static const uint8_t ModelDataRootIndex = 0;
        static const uint8_t DirectionalDataRootIndex = 1;
        static const uint8_t StructuredBufferShadowRootIndex = 0;

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

            mask_to_pso[filter_objects_]
                .set_vs(ShaderManager::GetShaderByName(CascadeShadowShader))
                .set_gs(ShaderManager::GetShaderByName(CascadeShadowGeomShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_dsv_format(DXGI_FORMAT_D32_FLOAT)
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
            auto cam_info = getCamInfoDataFunction_();

            context.SetSignature(mask_to_pso.begin()->second.GetSignature().get());

            std::unordered_map<ObjectMask, DrawPacket> lights;
            SceneVisibility::ExpandByFilterFromAll(filter_lights_, lights);

            for (const auto& [mask, lightPacket] : lights)
            {
                for (const auto lightGroup : lightPacket.common_resource_renderables)
                {
                    for (const auto rendLight : lightGroup.second.renderables)
                    {
                        // Should be only Directional light
                        const auto light = std::dynamic_pointer_cast<LightComponent>(rendLight.lock());
                        if (!light) continue;

                        D3D12_GPU_DESCRIPTOR_HANDLE shadow_srv;
                        D3D12_GPU_DESCRIPTOR_HANDLE light_srv;
                        if (const auto dirLight = std::dynamic_pointer_cast<DirectionalLightComponent>(light))
                        {
                            const auto dsv = dirLight->GetShadowDSV();
                            context.GetGraphicsCommandList()->OMSetRenderTargets(0, nullptr, true, &dsv);
                            dirLight->TransitionDepthShadowResource(context, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                            dirLight->ClearShadowDSV(context);
                            dirLight->TransitionDepthShadowResource(context, D3D12_RESOURCE_STATE_DEPTH_READ);
                            dirLight->UpdateCascadeGPUData(context, cam_info.camera);
                            shadow_srv = dirLight->GetCascadeDataSRV();
                            light_srv = dirLight->GetLightDataSRV();
                        }
                        else continue;

                        const auto lightViews = light->GetViews();
                        std::unordered_map<ObjectMask, DrawPacket> visibles;

                        for (const auto view : lightViews)
                        {
                            SceneVisibility::ExpandByFilterFromFrustum(filter_objects_, view, visibles);
                        }

                        for (auto& visible : visibles)
                        {
                            PSO pso;
                            if (!GetPsoFromMapByMask(mask_to_pso, visible.first, pso)) continue;
                            context.SetSignature(pso.GetSignature().get());
                            context.BindPSO(pso.GetState().get());
                            context.BindDescriptorHandle(ConstantBufferCount + StructuredBufferShadowRootIndex, shadow_srv);
                            context.BindDescriptorHandle(DirectionalDataRootIndex, light_srv);

                            for (auto& common_resource_group : visible.second.common_resource_renderables)
                            {
                                //auto CRG = common_resource_group.second;
                                //pso.SetShaderResources(context, *CRG.shaderResource);
                                for (auto& renderable : common_resource_group.second.renderables)
                                {
                                    pso.SetPerObjectData(context, renderable.lock()->GetPerObjectData());
                                    renderable.lock()->Draw(context);
                                }
                            }
                        }
                    }
                }
            }
        }

    private:
        ObjectMask filter_objects_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                 .SetBlendMode(BlendMode::Opaque | BlendMode::Masked)
                                                 .SetShadingModel(ShadingModel::DefaultLit)
                                                 .SetFillMode(FillMode::Solid);

        ObjectMask filter_lights_ = ObjectMask().SetLightType(LightType::Directional)
                                                .SetShadingModel(ShadingModel::All)
                                                .SetVertexType(VertexTypes::All)
                                                .SetFillMode(FillMode::All);


        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
    };
}
