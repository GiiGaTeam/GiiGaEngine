#pragma once

// GBufferPass.h — geometry pass of the deferred pipeline (NVRHI).
//
// Renders DefaultLit opaque solid geometry into the GBuffer (LightAccumulation, Diffuse,
// Material, NormalWS) + depth, and a wireframe variant. Bindings:
//   b0 = per-view constants, b1 = per-object constants (volatile, per draw),
//   b2 = material constants, t0..t7 = BaseColor,Metallic,Specular,Roughness,Anisotropy,
//        EmissiveColor,Opacity,Normal, s0 = linear-wrap sampler.
// Shaders: VertexPNTBTShader(VSMain) + GBufferOpaqueDefaultLitPixelShader(PSMain) for lit,
//          VertexPNTBTShader(VSMain) + GBufferWireframeShader(PSMain) for wire.

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
    class GBufferPass : public rhi::RenderPass
    {
    public:
        GBufferPass(nvrhi::IDevice* device, rhi::ShaderCache& shaders,
                    std::function<RenderPassViewData()> getCamInfo,
                    std::shared_ptr<rhi::GBuffer> gbuffer)
            : device_(device)
            , shaders_(shaders)
            , getCamInfoDataFunction_(std::move(getCamInfo))
            , gbuffer_(std::move(gbuffer))
        {
            viewSlot_.init(device_);
            objectSlot_.init(device_);

            nvrhi::SamplerHandle sampler = rhi::getLinearWrapSampler(device_);
            materialLayout_ = rhi::makeMaterialBindingLayout(device_, {0, 1, 2, 3, 4, 5, 6, 7}, true);
            sampler_ = sampler;
            createNullResources();
            nullMaterialSet_ = createNullMaterialSet();

            nvrhi::InputLayoutHandle inputLayout = rhi::createInputLayout(
                device_, rhi::makeVertexAttributes<VertexPNTBT>(),
                shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex));

            nvrhi::RenderState litState;
            litState.rasterState.setCullBack().setFrontCounterClockwise(true);
            litState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::Less).enableDepthTest().enableDepthWrite();

            nvrhi::GraphicsPipelineDesc litPd;
            litPd.setVertexShader(shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex))
                 .setPixelShader(shaders_.getOrCreate(device_, GBufferOpaqueDefaultLitShader, "PSMain", nvrhi::ShaderType::Pixel))
                 .setInputLayout(inputLayout)
                 .setRenderState(litState)
                 .addBindingLayout(viewSlot_.layout)
                 .addBindingLayout(objectSlot_.layout)
                 .addBindingLayout(materialLayout_);
            litPipeline_ = device_->createGraphicsPipeline(litPd, gbuffer_ ? gbuffer_->getFramebuffer() : nullptr);

            nvrhi::RenderState wireState;
            wireState.rasterState.setFillWireframe().setCullNone().setFrontCounterClockwise(true);
            wireState.depthStencilState.setDepthFunc(nvrhi::ComparisonFunc::Less).enableDepthTest().enableDepthWrite();

            nvrhi::GraphicsPipelineDesc wirePd;
            wirePd.setVertexShader(shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex))
                  .setPixelShader(shaders_.getOrCreate(device_, GBufferWireframeShader, "PSMain", nvrhi::ShaderType::Pixel))
                  .setInputLayout(inputLayout)
                  .setRenderState(wireState)
                  .addBindingLayout(viewSlot_.layout)
                  .addBindingLayout(objectSlot_.layout);
            wirePipeline_ = device_->createGraphicsPipeline(wirePd, gbuffer_ ? gbuffer_->getFramebuffer() : nullptr);
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            if (!ctx.cmd || !gbuffer_ || !litPipeline_) return;

            auto cam_info = getCamInfoDataFunction_();
            viewSlot_.write(ctx.cmd, cam_info.viewConstants);

            const auto& frustum = SceneVisibility::FrustumCulling(cam_info.camera.GetViewProj());
            const auto& vis_lit = SceneVisibility::Extract(filter_lit_solid_, frustum);
            const auto& vis_wire = SceneVisibility::Extract(filter_wire_, frustum);
            std::unordered_map<ObjectMask, DrawPacket> visibles;
            SceneVisibility::Expand(vis_lit, visibles);
            SceneVisibility::Expand(vis_wire, visibles);

            gbuffer_->clearAll(ctx.cmd);

            nvrhi::IFramebuffer* fb = gbuffer_->getFramebuffer();
            auto vp = rhi::fullViewportState(gbuffer_->width(), gbuffer_->height());

            for (auto& [mask, packet] : visibles)
            {
                const bool isWire = filter_wire_.CoversMask(mask);
                nvrhi::GraphicsPipelineHandle pipe = isWire ? wirePipeline_ : litPipeline_;
                if (!pipe) continue;

                for (auto& [key, group] : packet.common_resource_renderables)
                {
                    nvrhi::BindingSetHandle materialSet = nullMaterialSet_;
                    if (!isWire && group.shaderResource)
                    {
                        auto sresIt = materialSetCache_.find(group.shaderResource.get());
                        if (sresIt != materialSetCache_.end())
                            materialSet = sresIt->second;
                        else
                        {
                            materialSet = rhi::makeMaterialBindingSet(device_, materialLayout_,
                                group.shaderResource->GetMaterialBindings(), sampler_, {0, 1, 2, 3, 4, 5, 6, 7},
                                nullCb_.Get(), nullTex_.Get());
                            materialSetCache_[group.shaderResource.get()] = materialSet;
                        }
                    }

                    nvrhi::GraphicsState state;
                    state.setPipeline(pipe).setFramebuffer(fb).setViewport(vp);
                    state.addBindingSet(viewSlot_.set);
                    state.addBindingSet(objectSlot_.set);
                    if (!isWire) state.addBindingSet(materialSet);

                    for (auto& weak : group.renderables)
                    {
                        if (auto r = weak.lock())
                            rhi::drawRenderable(ctx.cmd, state, objectSlot_, *r);
                    }
                }
            }
        }

    private:
        ObjectMask filter_lit_solid_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                   .SetShadingModel(ShadingModel::DefaultLit)
                                                   .SetBlendMode(BlendMode::Opaque)
                                                   .SetFillMode(FillMode::Solid);

        ObjectMask filter_wire_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                              .SetShadingModel(ShadingModel::DefaultLit)
                                              .SetBlendMode(BlendMode::Opaque)
                                              .SetFillMode(FillMode::Wire);

        nvrhi::IDevice* device_ = nullptr;
        rhi::ShaderCache& shaders_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<rhi::GBuffer> gbuffer_;

        rhi::ViewConstantSlot viewSlot_;
        rhi::ObjectConstantSlot objectSlot_;
        nvrhi::BindingLayoutHandle materialLayout_;
        nvrhi::SamplerHandle sampler_;
        nvrhi::BindingSetHandle nullMaterialSet_;
        nvrhi::BufferHandle nullCb_;
        nvrhi::TextureHandle nullTex_;
        std::unordered_map<void*, nvrhi::BindingSetHandle> materialSetCache_;

        nvrhi::GraphicsPipelineHandle litPipeline_;
        nvrhi::GraphicsPipelineHandle wirePipeline_;

        void createNullResources()
        {
            nvrhi::BufferDesc cbd;
            cbd.setByteSize(256).setIsConstantBuffer(true)
               .setInitialState(nvrhi::ResourceStates::ConstantBuffer).setKeepInitialState(true)
               .setDebugName("gbuffer null material cb");
            nullCb_ = device_->createBuffer(cbd);

            nvrhi::TextureDesc td;
            td.setWidth(1).setHeight(1).setFormat(nvrhi::Format::RGBA8_UNORM)
              .setInitialState(nvrhi::ResourceStates::ShaderResource).setKeepInitialState(true)
              .setDebugName("gbuffer null texture");
            nullTex_ = device_->createTexture(td);

            nvrhi::CommandListHandle cl = device_->createCommandList();
            cl->open();
            uint8_t zeros[4] = {0, 0, 0, 0};
            cl->writeTexture(nullTex_, 0, 0, zeros, 4);
            cl->close();
            device_->executeCommandList(cl.Get());
            device_->waitForIdle();
        }

        nvrhi::BindingSetHandle createNullMaterialSet()
        {
            nvrhi::BindingSetDesc sd;
            sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(rhi::kMaterialCBSlot, nullCb_));
            for (uint32_t i = 0; i < 8; ++i)
                sd.addItem(nvrhi::BindingSetItem::Texture_SRV(i, nullTex_));
            sd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            return device_->createBindingSet(sd, materialLayout_);
        }
    };
}
