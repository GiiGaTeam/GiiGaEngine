#pragma once

// DebugPass.h — wireframe debug pass (NVRHI).
//
// Bindings: b0 = per-view, b1 = per-object (volatile), b2 = per-resource color constant
//           buffer (e.g. the CollisionShaderResource color CB). No textures.
// Shaders: VertexPNTBTShader(VSMain) + BufferWireframeShader(PSMain).
// Wireframe, no culling, into the GBuffer-derived light framebuffer.

#include <memory>
#include <functional>
#include <unordered_map>

#include <Core/Render/Nvrhi/NvrhiRenderPass.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiShader.h>
#include <Core/Render/Nvrhi/NvrhiResources.h>
#include <Core/Render/Nvrhi/NvrhiPassCommon.h>

#include <RenderPassViewData.h>
#include <SceneVisibility.h>
#include <IRenderable.h>
#include <ObjectMask.h>
#include <VertexTypes.h>

namespace GiiGa
{
    class DebugPass : public rhi::RenderPass
    {
    public:
        DebugPass(nvrhi::IDevice* device, rhi::ShaderCache& shaders,
                  std::function<RenderPassViewData()> getCamInfo,
                  std::shared_ptr<rhi::GBuffer> gbuffer)
            : device_(device)
            , shaders_(shaders)
            , getCamInfoDataFunction_(std::move(getCamInfo))
            , gbuffer_(std::move(gbuffer))
        {
            viewSlot_.init(device_);
            objectSlot_.init(device_);
            materialLayout_ = rhi::makeMaterialBindingLayout(device_, {}, false);

            nvrhi::InputLayoutHandle inputLayout = rhi::createInputLayout(
                device_, rhi::makeVertexAttributes<VertexPNTBT>(),
                shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex));

            nvrhi::RenderState rs;
            rs.rasterState.setFillWireframe().setCullNone();
            rs.depthStencilState.enableDepthTest().enableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Less);

            nvrhi::IFramebuffer* initialFb = rhi::ensureLightFramebuffer(device_, *gbuffer_, lightFb_, lightFbSource_);

            nvrhi::GraphicsPipelineDesc pd;
            pd.setVertexShader(shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex))
              .setPixelShader(shaders_.getOrCreate(device_, BufferWireframeShader, "PSMain", nvrhi::ShaderType::Pixel))
              .setInputLayout(inputLayout)
              .setRenderState(rs)
              .addBindingLayout(viewSlot_.layout)
              .addBindingLayout(objectSlot_.layout)
              .addBindingLayout(materialLayout_);
            pipeline_ = device_->createGraphicsPipeline(pd, initialFb);
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            if (!ctx.cmd || !gbuffer_ || !pipeline_) return;

            auto cam_info = getCamInfoDataFunction_();
            viewSlot_.write(ctx.cmd, cam_info.viewConstants);

            const auto& visibles = SceneVisibility::ExtractFromFrustum(filter_renderPass_, cam_info.camera.GetViewProj());

            nvrhi::IFramebuffer* fb = rhi::ensureLightFramebuffer(device_, *gbuffer_, lightFb_, lightFbSource_);
            auto vp = rhi::fullViewportState(gbuffer_->width(), gbuffer_->height());

            for (auto& [mask, packet] : visibles)
            {
                for (auto& [key, group] : packet.common_resource_renderables)
                {
                    nvrhi::BindingSetHandle materialSet;
                    if (group.shaderResource)
                    {
                        auto it = materialSetCache_.find(group.shaderResource.get());
                        if (it != materialSetCache_.end())
                            materialSet = it->second;
                        else
                        {
                            materialSet = rhi::makeMaterialBindingSet(device_, materialLayout_,
                                group.shaderResource->GetMaterialBindings(), nullptr, {});
                            materialSetCache_[group.shaderResource.get()] = materialSet;
                        }
                    }

                    nvrhi::GraphicsState state;
                    state.setPipeline(pipeline_).setFramebuffer(fb).setViewport(vp);
                    state.addBindingSet(viewSlot_.set);
                    state.addBindingSet(objectSlot_.set);
                    if (materialSet) state.addBindingSet(materialSet);

                    for (auto& weak : group.renderables)
                    {
                        if (auto r = weak.lock())
                            rhi::drawRenderable(ctx.cmd, state, objectSlot_, *r);
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

        nvrhi::IDevice* device_ = nullptr;
        rhi::ShaderCache& shaders_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<rhi::GBuffer> gbuffer_;

        rhi::ViewConstantSlot viewSlot_;
        rhi::ObjectConstantSlot objectSlot_;
        nvrhi::BindingLayoutHandle materialLayout_;
        std::unordered_map<void*, nvrhi::BindingSetHandle> materialSetCache_;

        nvrhi::FramebufferHandle lightFb_;
        nvrhi::IFramebuffer* lightFbSource_ = nullptr;

        nvrhi::GraphicsPipelineHandle pipeline_;
    };
}
