#pragma once

// ForwardPass.h — forward pass for unlit-opaque and translucent/masked geometry (NVRHI).
//
// Bindings: b0 = per-view, b1 = per-object (volatile), b2 = material constants,
//           t5 = EmissiveColor, s0 = linear-wrap sampler.
// Shaders: VertexPNTBTShader(VSMain) + OpaqueUnlitPixelShader(PSMain).
// Two pipelines share the same shaders: opaque (no blend) and translucent (src-alpha blend).
// Renders into the GBuffer-derived light framebuffer (LightAccumulation + depth).

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
    class ForwardPass : public rhi::RenderPass
    {
    public:
        ForwardPass(nvrhi::IDevice* device, rhi::ShaderCache& shaders,
                    std::function<RenderPassViewData()> getCamInfo,
                    std::shared_ptr<rhi::GBuffer> gbuffer)
            : device_(device)
            , shaders_(shaders)
            , getCamInfoDataFunction_(std::move(getCamInfo))
            , gbuffer_(std::move(gbuffer))
        {
            viewSlot_.init(device_);
            objectSlot_.init(device_);
            sampler_ = rhi::getLinearWrapSampler(device_);
            materialLayout_ = rhi::makeMaterialBindingLayout(device_, {5}, true);

            nvrhi::InputLayoutHandle inputLayout = rhi::createInputLayout(
                device_, rhi::makeVertexAttributes<VertexPNTBT>(),
                shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex));

            nvrhi::ShaderHandle vs = shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex);
            nvrhi::ShaderHandle ps = shaders_.getOrCreate(device_, OpaqueUnlitShader, "PSMain", nvrhi::ShaderType::Pixel);

            nvrhi::IFramebuffer* initialFb = rhi::ensureLightFramebuffer(device_, *gbuffer_, lightFb_, lightFbSource_);

            auto makePipe = [&](nvrhi::RenderState rs) {
                nvrhi::GraphicsPipelineDesc pd;
                pd.setVertexShader(vs).setPixelShader(ps).setInputLayout(inputLayout).setRenderState(rs);
                pd.addBindingLayout(viewSlot_.layout)
                  .addBindingLayout(objectSlot_.layout)
                  .addBindingLayout(materialLayout_);
                return device_->createGraphicsPipeline(pd, initialFb);
            };

            {
                nvrhi::RenderState rs;
                rs.rasterState.setCullBack().setFrontCounterClockwise(true);
                rs.depthStencilState.enableDepthTest().enableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Less);
                opaquePipeline_ = makePipe(rs);
            }
            {
                nvrhi::RenderState rs;
                rs.rasterState.setCullBack().setFrontCounterClockwise(true);
                rs.depthStencilState.enableDepthTest().disableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Less);
                nvrhi::BlendState::RenderTarget rt;
                rt.enableBlend()
                  .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
                  .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
                  .setBlendOp(nvrhi::BlendOp::Add)
                  .setSrcBlendAlpha(nvrhi::BlendFactor::InvDstAlpha)
                  .setDestBlendAlpha(nvrhi::BlendFactor::One)
                  .setBlendOpAlpha(nvrhi::BlendOp::Add);
                rs.blendState.setRenderTarget(0, rt);
                translucentPipeline_ = makePipe(rs);
            }
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            if (!ctx.cmd || !gbuffer_) return;

            auto cam_info = getCamInfoDataFunction_();
            viewSlot_.write(ctx.cmd, cam_info.viewConstants);

            const auto& frustum = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());
            const auto& vis_unlit = SceneVisibility::Extract(filter_unlit_solid_, frustum);
            const auto& vis_translucent = SceneVisibility::Extract(filter_translucent_, frustum);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_unlit, visibles);
            SceneVisibility::Expand(vis_translucent, visibles);

            nvrhi::IFramebuffer* fb = rhi::ensureLightFramebuffer(device_, *gbuffer_, lightFb_, lightFbSource_);
            auto vp = rhi::fullViewportState(gbuffer_->width(), gbuffer_->height());

            for (auto& [mask, packet] : visibles)
            {
                const bool isTranslucent = !filter_unlit_solid_.CoversMask(mask);
                nvrhi::GraphicsPipelineHandle pipe = isTranslucent ? translucentPipeline_ : opaquePipeline_;
                if (!pipe) continue;

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
                                group.shaderResource->GetMaterialBindings(), sampler_, {5});
                            materialSetCache_[group.shaderResource.get()] = materialSet;
                        }
                    }

                    nvrhi::GraphicsState state;
                    state.setPipeline(pipe).setFramebuffer(fb).setViewport(vp);
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
        ObjectMask filter_unlit_solid_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                     .SetShadingModel(ShadingModel::Unlit)
                                                     .SetBlendMode(BlendMode::Opaque)
                                                     .SetFillMode(FillMode::Solid);

        ObjectMask filter_translucent_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                      .SetShadingModel(ShadingModel::All)
                                                      .SetBlendMode(BlendMode::Translucent | BlendMode::Masked)
                                                      .SetFillMode(FillMode::All);

        nvrhi::IDevice* device_ = nullptr;
        rhi::ShaderCache& shaders_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<rhi::GBuffer> gbuffer_;

        rhi::ViewConstantSlot viewSlot_;
        rhi::ObjectConstantSlot objectSlot_;
        nvrhi::BindingLayoutHandle materialLayout_;
        nvrhi::SamplerHandle sampler_;
        std::unordered_map<void*, nvrhi::BindingSetHandle> materialSetCache_;

        nvrhi::FramebufferHandle lightFb_;
        nvrhi::IFramebuffer* lightFbSource_ = nullptr;

        nvrhi::GraphicsPipelineHandle opaquePipeline_;
        nvrhi::GraphicsPipelineHandle translucentPipeline_;
    };
}
