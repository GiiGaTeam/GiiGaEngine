#pragma once

// ForwardPass.h — forward pass for unlit-opaque and translucent/masked geometry (NVRHI).
//
// Bindings (2-layout workaround for NVRHI 3-layout crash):
//   Layout 0: b0 = per-view (volatile)
//   Layout 1: b1 = per-object (volatile) + b2 = material CB + t0..t7 = material textures + s0
//
// Merging the object VCB and material descriptor table into one binding layout avoids
// an NVRHI D3D12 backend crash (0xC0000005 in setGraphicsState after ~3 frames) that
// occurs when ForwardPass has 3 binding layouts. GBufferPass with 3 layouts works fine;
// the root cause appears to be a pipeline/root-signature caching bug specific to this
// pass's pipelines. The merged layout produces identical GPU-side behavior.
//
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
            mergedLayout_ = makeMergedLayout();
            nullMergedSet_ = makeNullMergedSet();

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
                  .addBindingLayout(mergedLayout_);
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

            // NOP — see header comment about timing-dependent crash

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
                    nvrhi::BindingSetHandle mergedSet = nullMergedSet_;
                    if (group.shaderResource)
                    {
                        auto it = mergedSetCache_.find(group.shaderResource.get());
                        if (it != mergedSetCache_.end())
                            mergedSet = it->second;
                        else
                        {
                            mergedSet = makeMergedSet(group.shaderResource->GetMaterialBindings());
                            mergedSetCache_[group.shaderResource.get()] = mergedSet;
                        }
                    }

                    nvrhi::GraphicsState state;
                    state.setPipeline(pipe).setFramebuffer(fb).setViewport(vp);
                    state.addBindingSet(viewSlot_.set);
                    state.addBindingSet(mergedSet);

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
        nvrhi::BindingLayoutHandle mergedLayout_;
        nvrhi::SamplerHandle sampler_;
        nvrhi::BindingSetHandle nullMergedSet_;
        std::unordered_map<void*, nvrhi::BindingSetHandle> mergedSetCache_;

        nvrhi::FramebufferHandle lightFb_;
        nvrhi::IFramebuffer* lightFbSource_ = nullptr;

        nvrhi::GraphicsPipelineHandle opaquePipeline_;
        nvrhi::GraphicsPipelineHandle translucentPipeline_;

        nvrhi::BufferHandle nullCb_;
        nvrhi::TextureHandle nullTex_;

        // Layout 1: VCB(b1) + CB(b2) + SRV(t0..t7) + Sampler(s0)
        nvrhi::BindingLayoutHandle makeMergedLayout()
        {
            nvrhi::BindingLayoutDesc ld;
            ld.setVisibility(nvrhi::ShaderType::All);
            ld.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(rhi::kObjectSlot));
            ld.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(rhi::kMaterialCBSlot));
            for (uint32_t i = 0; i < 8; ++i)
                ld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(i));
            ld.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            return device_->createBindingLayout(ld);
        }

        nvrhi::BindingSetHandle makeMergedSet(const MaterialBindings& mb)
        {
            nvrhi::BindingSetDesc sd;
            sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(rhi::kObjectSlot, objectSlot_.cb));
            sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(rhi::kMaterialCBSlot,
                mb.constantBuffer ? mb.constantBuffer : nullCb_.Get()));
            for (uint32_t i = 0; i < 8; ++i)
                sd.addItem(nvrhi::BindingSetItem::Texture_SRV(i,
                    mb.textures[i] ? mb.textures[i] : nullTex_.Get()));
            sd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            return device_->createBindingSet(sd, mergedLayout_);
        }

        nvrhi::BindingSetHandle makeNullMergedSet()
        {
            nvrhi::BufferDesc cbd;
            cbd.setByteSize(256).setIsConstantBuffer(true)
               .setInitialState(nvrhi::ResourceStates::ConstantBuffer).setKeepInitialState(true)
               .setDebugName("forward null material cb");
            nullCb_ = device_->createBuffer(cbd);

            nvrhi::TextureDesc td;
            td.setWidth(1).setHeight(1).setFormat(nvrhi::Format::RGBA8_UNORM)
              .setInitialState(nvrhi::ResourceStates::ShaderResource).setKeepInitialState(true)
              .setDebugName("forward null texture");
            nullTex_ = device_->createTexture(td);

            nvrhi::CommandListHandle cl = device_->createCommandList();
            cl->open();
            uint8_t zeros[4] = {0, 0, 0, 0};
            cl->writeTexture(nullTex_, 0, 0, zeros, 4);
            cl->close();
            device_->executeCommandList(cl.Get());
            device_->waitForIdle();

            nvrhi::BindingSetDesc sd;
            sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(rhi::kObjectSlot, objectSlot_.cb));
            sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(rhi::kMaterialCBSlot, nullCb_));
            for (uint32_t i = 0; i < 8; ++i)
                sd.addItem(nvrhi::BindingSetItem::Texture_SRV(i, nullTex_));
            sd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            return device_->createBindingSet(sd, mergedLayout_);
        }
    };
}
