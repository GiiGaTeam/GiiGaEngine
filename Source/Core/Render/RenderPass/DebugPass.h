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

namespace GiiGa
{
    class DebugPass : public RenderPass
    {
    public:
        static inline const int ViewDataRootIndex = 0;
        static inline const int ModelDataRootIndex = 1;

        static inline const int ConstantBufferCount = ModelDataRootIndex + 1;

        DebugPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
            D3D12_RASTERIZER_DESC rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.CullMode = D3D12_CULL_MODE_NONE;
            rast_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;

            DXGI_FORMAT format_array[] = {GBuffer::G_FORMAT};

            mask_to_pso[filter_renderPass_]
                .set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader))
                .set_ps(ShaderManager::GetShaderByName(BufferWireframeShader))
                .set_rasterizer_state(rast_desc)
                .set_rtv_format(format_array, 1)
                .set_input_layout(VertexPNTBT::InputLayout)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                })
                .GeneratePSO(context.GetDevice(), ConstantBufferCount, MaxTextureCount);
        }

        void Draw(RenderContext& context) override
        {
            context.GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            auto cam_info = getCamInfoDataFunction_();

            const auto& visibles = SceneVisibility::ExtractFromFrustum(filter_renderPass_, cam_info.camera.GetViewProj());

            context.SetSignature(mask_to_pso.begin()->second.GetSignature().get());
            context.BindDescriptorHandle(0, cam_info.viewDescriptor);

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
                                                    .SetShadingModel(ShadingModel::All)
                                                    .SetBlendMode(BlendMode::Debug)
                                                    .SetFillMode(FillMode::All)
                                                    .SetLightType(LightType::NoDirectional);

        std::unordered_map<ObjectMask, PSO> mask_to_pso;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
    };
}
