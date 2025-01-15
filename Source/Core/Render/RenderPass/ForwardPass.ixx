module;

#include<directxtk12/SimpleMath.h>
#include<d3d12.h>
#include<directx/d3dx12_core.h>

export module ForwardPass;

import <memory>;
import <vector>;
import <functional>;

import RenderPass;
import PSO;
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

namespace GiiGa
{
    export class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
            D3D12_RASTERIZER_DESC rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.CullMode = D3D12_CULL_MODE_NONE;

            mask_to_pso[filter_renderPass_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(GBufferOpaqueUnlitShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(1, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                })
                .GeneratePSO(context.GetDevice(), 2);
        }

        void Draw(RenderContext& context) override
        {
            context.GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            auto cam_info = getCamInfoDataFunction_();

            const auto& visibles = SceneVisibility::ExtractFromFrustum(filter_renderPass_, cam_info.camera.GetViewProj());

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
        ObjectMask filter_renderPass_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                    .SetShadingModel(ShadingModel::Unlit)
                                                    .SetBlendMode(BlendMode::Opaque);

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
    };
}
