#pragma once

#include<pybind11/pybind11.h>
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
        ShadowPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
            nvrhi::RasterState rast_state;
            rast_state.fillMode = nvrhi::RasterFillMode::Solid;
            rast_state.cullMode = nvrhi::RasterCullMode::Back;
            rast_state.frontCounterClockwise = true;
            rast_state.depthBias = DepthBias;
            rast_state.depthBiasClamp = DepthBiasClamp;
            rast_state.slopeScaledDepthBias = SlopeScaledDepthBias;

            nvrhi::DepthStencilState depth_stencil_state;
            depth_stencil_state.depthTestEnable = true;
            depth_stencil_state.depthWriteEnable = true;
            depth_stencil_state.depthFunc = nvrhi::ComparisonFunc::Less;
            depth_stencil_state.stencilEnable = false;

            mask_to_pso[filter_objects_]
                .set_vs(ShaderManager::GetShaderByName(CascadeShadowShader))
                .set_gs(ShaderManager::GetShaderByName(CascadeShadowGeomShader))
                .set_rasterizer_state(rast_state)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_depth_stencil_state(depth_stencil_state)
                // registers of CascadeShadowShader (VS) + CascadeShadowGeomShader (GS)
                .add_constant_buffer(0)       // per-object world matrices
                .add_constant_buffer(1)       // directional light data
                .add_structured_buffer_srv(0) // cascade data
                .set_per_object_slot(0)
                .GeneratePSO(context.GetDevice());
        }

        void Draw(RenderContext& context) override
        {
            ZoneScoped;
            GIIGA_GPU_ZONE(context, "Shadow");
            auto cam_info = getCamInfoDataFunction_();

            std::unordered_map<ObjectMask, DrawPacket> lights;
            SceneVisibility::ExpandByFilterFromAll(filter_lights_, lights);

            for (auto& [mask, lightPacket] : lights)
            {
                for (const auto& lightGroup : lightPacket.common_resource_renderables)
                {
                    for (const auto& rendLight : lightGroup.second.renderables)
                    {
                        // Should be only Directional light
                        const auto light = std::dynamic_pointer_cast<LightComponent>(rendLight.lock());
                        if (!light) continue;

                        const auto dirLight = std::dynamic_pointer_cast<DirectionalLightComponent>(light);
                        if (!dirLight) continue;

                        dirLight->ClearShadowMap(context);
                        dirLight->UpdateCascadeGPUData(context, cam_info.camera);

                        context.SetFramebuffer(dirLight->GetShadowFramebuffer());
                        context.SetViewport(dirLight->GetShadowMapSize().x, dirLight->GetShadowMapSize().y);

                        const auto lightViews = light->GetViews();
                        std::unordered_map<ObjectMask, DrawPacket> visibles;

                        for (const auto& view : lightViews)
                        {
                            SceneVisibility::ExpandByFilterFromFrustum(filter_objects_, view, visibles);
                        }

                        for (auto& visible : visibles)
                        {
                            PSO* pso = GetPsoFromMapByMask(mask_to_pso, visible.first);
                            if (!pso) continue;

                            context.SetPipeline(pso->GetOrCreatePipeline(context.GetDevice(), dirLight->GetShadowFramebuffer()));

                            for (auto& common_resource_group : visible.second.common_resource_renderables)
                            {
                                for (auto& renderable : common_resource_group.second.renderables)
                                {
                                    nvrhi::BindingSetDesc binding_desc;
                                    binding_desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, dirLight->GetLightDataCB()));
                                    binding_desc.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(0, dirLight->GetCascadeDataBuffer()));
                                    pso->AppendPerObjectData(binding_desc, renderable.lock()->GetPerObjectData());

                                    context.SetBindingSet(context.GetOrCreateBindingSet(binding_desc, pso->GetBindingLayout()));
                                    renderable.lock()->Draw(context);
                                }
                            }
                        }
                    }
                }
            }

            // restore the camera viewport
            context.SetViewport(cam_info.screenDimensions.screenDimensions.x, cam_info.screenDimensions.screenDimensions.y);
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
        int DepthBias = -15000;
        float DepthBiasClamp = 0;
        float SlopeScaledDepthBias = 8.000;
    };
}
