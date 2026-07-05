#pragma once

// GLightPass.h — deferred lighting pass with a two-phase stencil-volume algorithm (NVRHI).
//
// Bindings: b0 = per-view, b1 = per-object (volatile), b2 = screen dimensions (volatile),
//           b3 = per-light constants.
//   point (GPointLight.hlsl):        t0 Diffuse, t1 Material, t2 NormalWS; s0 linear.
//   directional (GDirectionalLight): t0 Diffuse, t1 Material, t2 NormalWS,
//                                    t3 CascadeShadowMaps (Texture2DArray),
//                                    t4 cascadeData (StructuredBuffer<CascadeData>);
//                                    s0 linear, s1 comparison.
//
// Per light: clear stencil to 1, run "unmark" (depth-only, DECR_SAT) for point lights, then
// "shade" (additive blend, stencil EQUAL) into LightAccumulation.
//   point:        VertexPNTBTShader(VSMain) + GPointLight(PSMain)
//   directional:  VertexFullQuadShader(VSMain) + GDirectionalLight(PSMain)

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
#include <DirectionalLightComponent.h>

namespace GiiGa
{
    class GLightPass : public rhi::RenderPass
    {
    public:
        GLightPass(nvrhi::IDevice* device, rhi::ShaderCache& shaders,
                   std::function<RenderPassViewData()> getCamInfo,
                   std::shared_ptr<rhi::GBuffer> gbuffer)
            : device_(device)
            , shaders_(shaders)
            , getCamInfoDataFunction_(std::move(getCamInfo))
            , gbuffer_(std::move(gbuffer))
        {
            viewSlot_.init(device_);
            objectSlot_.init(device_);
            screenDimSlot_.init(device_, rhi::kMaterialCBSlot, sizeof(rhi::ScreenDimensions), "screen dims vcb");
            linearSampler_ = rhi::getLinearWrapSampler(device_);
            comparisonSampler_ = rhi::getComparisonSampler(device_);

            // Point-light layout (Pixel): b3 light CB + t0..t2 GBuffer SRVs + s0 sampler.
            {
                nvrhi::BindingLayoutDesc pld;
                pld.setVisibility(nvrhi::ShaderType::Pixel);
                pld.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(3));
                pld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
                pld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(1));
                pld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(2));
                pld.addItem(nvrhi::BindingLayoutItem::Sampler(0));
                pointLayout_ = device_->createBindingLayout(pld);
            }

            // Directional-light layout (Pixel): b3 light CB + t0..t2 GBuffer SRVs
            // + t3 shadow array (Texture_SRV) + t4 cascade StructuredBuffer + s0,s1 samplers.
            {
                nvrhi::BindingLayoutDesc dld;
                dld.setVisibility(nvrhi::ShaderType::Pixel);
                dld.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(3));
                dld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
                dld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(1));
                dld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(2));
                dld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(3));
                dld.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(4));
                dld.addItem(nvrhi::BindingLayoutItem::Sampler(0));
                dld.addItem(nvrhi::BindingLayoutItem::Sampler(1));
                dirLayout_ = device_->createBindingLayout(dld);
            }

            nvrhi::InputLayoutHandle inputLayout = rhi::createInputLayout(
                device_, rhi::makeVertexAttributes<VertexPNTBT>(),
                shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex));

            nvrhi::IFramebuffer* lightFb = rhi::ensureLightFramebuffer(device_, *gbuffer_, lightFb_, lightFbSource_);
            ensureDepthOnlyFb();

            auto additiveBlend = []() {
                nvrhi::BlendState::RenderTarget rt;
                rt.enableBlend()
                  .setSrcBlend(nvrhi::BlendFactor::One)
                  .setDestBlend(nvrhi::BlendFactor::One)
                  .setBlendOp(nvrhi::BlendOp::Add)
                  .setSrcBlendAlpha(nvrhi::BlendFactor::One)
                  .setDestBlendAlpha(nvrhi::BlendFactor::Zero)
                  .setBlendOpAlpha(nvrhi::BlendOp::Add);
                return rt;
            };

            // --- unmark pipeline: VS only, depth GREATER, stencil DECR_SAT/ALWAYS, CullBack ---
            {
                nvrhi::RenderState rs;
                rs.rasterState.setFillSolid().setCullBack().setFrontCounterClockwise(true).enableDepthClip();
                nvrhi::DepthStencilState ds;
                ds.enableDepthTest().disableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Greater);
                ds.enableStencil().setStencilReadMask(0xff).setStencilWriteMask(0xff);
                nvrhi::DepthStencilState::StencilOpDesc sop;
                sop.setFailOp(nvrhi::StencilOp::Keep).setDepthFailOp(nvrhi::StencilOp::Keep)
                   .setPassOp(nvrhi::StencilOp::DecrementAndClamp).setStencilFunc(nvrhi::ComparisonFunc::Always);
                ds.setFrontFaceStencil(sop).setBackFaceStencil(sop);
                rs.setDepthStencilState(ds);

                nvrhi::GraphicsPipelineDesc pd;
                pd.setVertexShader(shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex))
                  .setInputLayout(inputLayout).setRenderState(rs);
                pd.addBindingLayout(viewSlot_.layout).addBindingLayout(objectSlot_.layout).addBindingLayout(screenDimSlot_.layout);
                unmarkPipeline_ = device_->createGraphicsPipeline(pd, depthOnlyFb_.Get());
            }

            // --- shade (point): VS+PS, CullFront, depth GREATER_OR_EQUAL, stencil EQUAL(ref 1) ---
            shadePointPipeline_ = makeShadePipeline(
                shaders_.getOrCreate(device_, VertexPNTBTShader, "VSMain", nvrhi::ShaderType::Vertex),
                shaders_.getOrCreate(device_, GPointLight, "PSMain", nvrhi::ShaderType::Pixel),
                inputLayout, pointLayout_, nvrhi::ComparisonFunc::GreaterOrEqual, additiveBlend(), lightFb);

            // --- shade (directional): full-quad VS, depth LESS ---
            shadeDirPipeline_ = makeShadePipeline(
                shaders_.getOrCreate(device_, VertexFullQuadShader, "VSMain", nvrhi::ShaderType::Vertex),
                shaders_.getOrCreate(device_, GDirectionLight, "PSMain", nvrhi::ShaderType::Pixel),
                inputLayout, dirLayout_, nvrhi::ComparisonFunc::Less, additiveBlend(), lightFb);
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            if (!ctx.cmd || !gbuffer_) return;

            auto cam_info = getCamInfoDataFunction_();
            viewSlot_.write(ctx.cmd, cam_info.viewConstants);
            screenDimSlot_.write(ctx.cmd, cam_info.screenDimensions);
            auto visibles = SceneVisibility::ExtractFromFrustum(filter_pointLight_, cam_info.camera.GetViewProj());
            SceneVisibility::ExpandByFilterFromAll(filter_directionalLight_, visibles);

            nvrhi::IFramebuffer* lightFb = rhi::ensureLightFramebuffer(device_, *gbuffer_, lightFb_, lightFbSource_);
            ensureDepthOnlyFb();
            auto vp = rhi::fullViewportState(gbuffer_->width(), gbuffer_->height());

            for (auto& [mask, packet] : visibles)
            {
                const bool isDirectional = filter_directionalLight_.CoversMask(mask);
                nvrhi::GraphicsPipelineHandle shadePipe = isDirectional ? shadeDirPipeline_ : shadePointPipeline_;
                if (!shadePipe) continue;

                for (auto& [key, group] : packet.common_resource_renderables)
                {
                    for (auto& weak : group.renderables)
                    {
                        auto r = weak.lock();
                        if (!r) continue;

                        nvrhi::BindingSetHandle lightSet;
                        if (isDirectional)
                        {
                            auto dirLight = std::dynamic_pointer_cast<DirectionalLightComponent>(r);
                            if (!dirLight || !dirLight->getShadowTexture() || !dirLight->getCascadeBuffer())
                                continue;
                            auto it = dirLightSetCache_.find(dirLight.get());
                            if (it != dirLightSetCache_.end())
                                lightSet = it->second;
                            else
                            {
                                lightSet = makeDirLightSet(*dirLight);
                                dirLightSetCache_[dirLight.get()] = lightSet;
                            }
                        }
                        else
                        {
                            auto it = pointLightSetCache_.find(group.shaderResource.get());
                            if (it != pointLightSetCache_.end())
                                lightSet = it->second;
                            else
                            {
                                lightSet = makePointLightSet(group.shaderResource->GetMaterialBindings());
                                pointLightSetCache_[group.shaderResource.get()] = lightSet;
                            }
                        }

                        ctx.cmd->clearDepthStencilTexture(gbuffer_->getDepth(), nvrhi::AllSubresources,
                                                          /*clearDepth=*/false, 0.f, /*clearStencil=*/true, 1);

                        if (!isDirectional && unmarkPipeline_)
                        {
                            nvrhi::GraphicsState unmarkState;
                            unmarkState.setPipeline(unmarkPipeline_).setFramebuffer(depthOnlyFb_.Get()).setViewport(vp);
                            unmarkState.addBindingSet(viewSlot_.set);
                            unmarkState.addBindingSet(objectSlot_.set);
                            unmarkState.addBindingSet(screenDimSlot_.set);
                            rhi::drawRenderable(ctx.cmd, unmarkState, objectSlot_, *r);
                        }

                        nvrhi::GraphicsState shadeState;
                        shadeState.setPipeline(shadePipe).setFramebuffer(lightFb).setViewport(vp);
                        shadeState.setDynamicStencilRefValue(1);
                        shadeState.addBindingSet(viewSlot_.set);
                        shadeState.addBindingSet(objectSlot_.set);
                        shadeState.addBindingSet(screenDimSlot_.set);
                        if (lightSet) shadeState.addBindingSet(lightSet);
                        rhi::drawRenderable(ctx.cmd, shadeState, objectSlot_, *r);
                    }
                }
            }
        }

    private:
        ObjectMask filter_pointLight_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                    .SetLightType(LightType::Point)
                                                    .SetBlendMode(BlendMode::Debug);

        ObjectMask filter_directionalLight_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                          .SetLightType(LightType::Directional)
                                                          .SetBlendMode(BlendMode::Debug);

        nvrhi::IDevice* device_ = nullptr;
        rhi::ShaderCache& shaders_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
        std::shared_ptr<rhi::GBuffer> gbuffer_;

        rhi::ViewConstantSlot viewSlot_;
        rhi::ObjectConstantSlot objectSlot_;
        rhi::VolatileConstantSlot screenDimSlot_;
        nvrhi::BindingLayoutHandle pointLayout_;
        nvrhi::BindingLayoutHandle dirLayout_;
        nvrhi::SamplerHandle linearSampler_;
        nvrhi::SamplerHandle comparisonSampler_;
        std::unordered_map<void*, nvrhi::BindingSetHandle> pointLightSetCache_;
        std::unordered_map<DirectionalLightComponent*, nvrhi::BindingSetHandle> dirLightSetCache_;

        nvrhi::FramebufferHandle lightFb_;
        nvrhi::IFramebuffer* lightFbSource_ = nullptr;
        nvrhi::FramebufferHandle depthOnlyFb_;
        nvrhi::IFramebuffer* depthOnlyFbSource_ = nullptr;

        nvrhi::GraphicsPipelineHandle unmarkPipeline_;
        nvrhi::GraphicsPipelineHandle shadePointPipeline_;
        nvrhi::GraphicsPipelineHandle shadeDirPipeline_;

        void ensureDepthOnlyFb()
        {
            if (depthOnlyFb_ && depthOnlyFbSource_ == gbuffer_->getFramebuffer()) return;
            nvrhi::FramebufferDesc fbd;
            fbd.setDepthAttachment(gbuffer_->getDepth());
            depthOnlyFb_ = device_->createFramebuffer(fbd);
            depthOnlyFbSource_ = gbuffer_->getFramebuffer();
        }

        nvrhi::BindingSetHandle makePointLightSet(const MaterialBindings& mb)
        {
            nvrhi::BindingSetDesc sd;
            if (mb.constantBuffer)
                sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(3, mb.constantBuffer));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gbuffer_->getColor(rhi::GBuffer::Order::Diffuse)));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(1, gbuffer_->getColor(rhi::GBuffer::Order::Material)));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(2, gbuffer_->getColor(rhi::GBuffer::Order::NormalWS)));
            sd.addItem(nvrhi::BindingSetItem::Sampler(0, linearSampler_));
            return device_->createBindingSet(sd, pointLayout_);
        }

        nvrhi::BindingSetHandle makeDirLightSet(DirectionalLightComponent& light)
        {
            const auto& mb = light.GetShaderResource()->GetMaterialBindings();
            nvrhi::BindingSetDesc sd;
            if (mb.constantBuffer)
                sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(3, mb.constantBuffer));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gbuffer_->getColor(rhi::GBuffer::Order::Diffuse)));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(1, gbuffer_->getColor(rhi::GBuffer::Order::Material)));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(2, gbuffer_->getColor(rhi::GBuffer::Order::NormalWS)));
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(3, light.getShadowTexture()));
            sd.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(4, light.getCascadeBuffer()));
            sd.addItem(nvrhi::BindingSetItem::Sampler(0, linearSampler_));
            sd.addItem(nvrhi::BindingSetItem::Sampler(1, comparisonSampler_));
            return device_->createBindingSet(sd, dirLayout_);
        }

        nvrhi::GraphicsPipelineHandle makeShadePipeline(nvrhi::IShader* vs, nvrhi::IShader* ps,
                                                        nvrhi::IInputLayout* inputLayout,
                                                        nvrhi::IBindingLayout* lightLayout,
                                                        nvrhi::ComparisonFunc depthFunc,
                                                        const nvrhi::BlendState::RenderTarget& blend,
                                                        nvrhi::IFramebuffer* fb)
        {
            nvrhi::RenderState rs;
            rs.rasterState.setFillSolid().setCullFront().setFrontCounterClockwise(true).disableDepthClip();
            nvrhi::DepthStencilState ds;
            ds.enableDepthTest().disableDepthWrite().setDepthFunc(depthFunc);
            ds.enableStencil().setStencilReadMask(0xff).setStencilWriteMask(0xff).setStencilRefValue(1);
            nvrhi::DepthStencilState::StencilOpDesc sop;
            sop.setFailOp(nvrhi::StencilOp::Keep).setDepthFailOp(nvrhi::StencilOp::Keep)
               .setPassOp(nvrhi::StencilOp::Keep).setStencilFunc(nvrhi::ComparisonFunc::Equal);
            ds.setFrontFaceStencil(sop).setBackFaceStencil(sop);
            rs.setDepthStencilState(ds);
            nvrhi::BlendState bs;
            bs.setRenderTarget(0, blend);
            rs.setBlendState(bs);

            nvrhi::GraphicsPipelineDesc pd;
            pd.setVertexShader(vs).setPixelShader(ps).setInputLayout(inputLayout).setRenderState(rs);
            pd.addBindingLayout(viewSlot_.layout)
              .addBindingLayout(objectSlot_.layout)
              .addBindingLayout(screenDimSlot_.layout)
              .addBindingLayout(lightLayout);
            return device_->createGraphicsPipeline(pd, fb);
        }
    };
}
