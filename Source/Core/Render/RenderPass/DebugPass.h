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

namespace GiiGa
{
    class DebugPass : public RenderPass
    {
    public:
        DebugPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            nvrhi::RasterState rast_state;
            rast_state.fillMode = nvrhi::RasterFillMode::Wireframe;
            rast_state.cullMode = nvrhi::RasterCullMode::None;

            nvrhi::DepthStencilState depth_stencil_state;
            depth_stencil_state.depthTestEnable = true;
            depth_stencil_state.depthWriteEnable = true;
            depth_stencil_state.depthFunc = nvrhi::ComparisonFunc::Less;
            depth_stencil_state.stencilEnable = false;

            mask_to_pso[filter_renderPass_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(BufferWireframeShader))
                .set_rasterizer_state(rast_state)
                .set_depth_stencil_state(depth_stencil_state)
                .set_input_layout(VertexPNTBT::InputLayout)
                .add_constant_buffer(0)
                .add_constant_buffer(1)
                .GeneratePSO(context.GetDevice());
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();

            auto visibles = SceneVisibility::ExtractFromFrustum(filter_renderPass_, cam_info.camera.GetViewProj());

            context.SetFramebuffer(gbuffer_->GetAccumFramebuffer());

            for (auto& visible : visibles)
            {
                PSO* pso = GetPsoFromMapByMask(mask_to_pso, visible.first);
                if (!pso) continue;

                context.SetPipeline(pso->GetOrCreatePipeline(context.GetDevice(), gbuffer_->GetAccumFramebuffer()));

                for (auto& common_resource_group : visible.second.common_resource_renderables)
                {
                    for (auto& renderable : common_resource_group.second.renderables)
                    {
                        nvrhi::BindingSetDesc binding_desc;
                        binding_desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewConstantBuffer));
                        pso->AppendPerObjectData(binding_desc, renderable.lock()->GetPerObjectData());

                        context.SetBindingSet(context.GetOrCreateBindingSet(binding_desc, pso->GetBindingLayout()));
                        renderable.lock()->Draw(context);
                    }
                }
            }
        }

    private:
        ObjectMask filter_renderPass_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                    .SetShadingModel(ShadingModel::All)
                                                    .SetBlendMode(BlendMode::Debug)
                                                    .SetFillMode(FillMode::All)
                                                    .SetLightType(LightType::NoDirectional);

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
