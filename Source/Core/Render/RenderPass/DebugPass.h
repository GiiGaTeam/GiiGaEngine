#pragma once

#include<memory>
#include<functional>
#include<unordered_map>

#include<nvrhi/nvrhi.h>

#include<RenderPass.h>
#include<Pipeline.h>
#include<IRenderable.h>
#include<SceneVisibility.h>
#include<RenderPassViewData.h>
#include<ShaderManager.h>
#include<RenderContext.h>
#include<VertexTypes.h>
#include<CameraComponent.h>
#include<PerObjectData.h>
#include<GBuffer.h>

namespace GiiGa
{
    // Wireframe debug pass: draws PNTBT geometry (any shading/fill, no directional lights) with a
    // flat wireframe pixel shader into LightAccumulation. Only view (b0) + object (b1) CBs bound.
    class DebugPass : public RenderPass
    {
    public:
        DebugPass(RenderContext& context, const std::function<RenderPassViewData()>& getCamDescFn, std::shared_ptr<GBuffer> gbuffer):
            getCamInfoDataFunction_(getCamDescFn),
            gbuffer_(gbuffer)
        {
            auto& device = context.GetDevice();

            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.setVisibility(nvrhi::ShaderType::All);
            layoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            layoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
            bindingLayout_ = device.Device()->createBindingLayout(layoutDesc);

            auto vs = ShaderManager::GetShaderByName(device, VertexPNTBTShader);

            Pipeline p;
            p.VS(vs)
             .PS(ShaderManager::GetShaderByName(device, BufferWireframeShader))
             .InputLayout(VertexPNTBT::CreateInputLayout(device, vs))
             .Raster(DefaultRasterState().setFillWireframe().setCullNone())
             .Depth(DefaultDepthState())
             .AddBindingLayout(bindingLayout_);

            mask_to_pipeline_[filter_renderPass_] = MaskPipeline{p.Build(device, gbuffer_->GetLightFramebuffer()), bindingLayout_};
        }

        void Draw(RenderContext& context) override
        {
            auto cam_info = getCamInfoDataFunction_();
            auto& device = context.GetDevice();

            const auto& visibles = SceneVisibility::ExtractFromFrustum(filter_renderPass_, cam_info.camera.GetViewProj());

            nvrhi::ViewportState vp;
            vp.addViewportAndScissorRect(nvrhi::Viewport(cam_info.screenDimensions.screenDimensions.x,
                                                         cam_info.screenDimensions.screenDimensions.y));

            for (auto& visible : visibles)
            {
                MaskPipeline mp;
                if (!GetPipelineFromMapByMask(mask_to_pipeline_, visible.first, mp)) continue;

                for (auto& [key, group] : visible.second.common_resource_renderables)
                {
                    for (auto& weak : group.renderables)
                    {
                        auto renderable = weak.lock();
                        if (!renderable) continue;

                        nvrhi::BindingSetDesc bsd;
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, cam_info.viewCB));
                        bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, renderable->GetPerObjectData().GetBuffer()));
                        nvrhi::BindingSetHandle set = device.Device()->createBindingSet(bsd, bindingLayout_);

                        nvrhi::GraphicsState state;
                        state.setPipeline(mp.pipeline).setFramebuffer(gbuffer_->GetLightFramebuffer()).setViewport(vp);
                        state.addBindingSet(set);

                        renderable->Draw(context, state);
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

        std::unordered_map<ObjectMask, MaskPipeline> mask_to_pipeline_;
        nvrhi::BindingLayoutHandle bindingLayout_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
