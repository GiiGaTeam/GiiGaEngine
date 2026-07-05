#pragma once

// ShadowPass.h — cascaded shadow-map generation for directional lights (NVRHI).
//
// Pipeline: CascadeShadowShader(VSMain) + CascadeShadowGeomShader(GSMain). The geometry shader
// amplifies each triangle to NUM_CASCADE slices (writing SV_RenderTargetArrayIndex), reading
// per-cascade View/Proj from the cascade StructuredBuffer<CascadeData> at t0.
//
// Bindings per draw:
//   b0  WorldMatrices          (volatile, per-object — ObjectConstantSlot)
//   b1  DirectionLightData     (per-light constant buffer from DirectionalLightComponent)
//   t0  CascadeData[]          (per-light StructuredBuffer from DirectionalLightComponent)
//
// Render target: the light's D32_FLOAT 2D-array depth texture (no color, no pixel shader),
// rendered with depth bias to mitigate shadow acne. The light owns its shadow framebuffer.

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
    class ShadowPass : public rhi::RenderPass
    {
    public:
        ShadowPass(nvrhi::IDevice* device, rhi::ShaderCache& shaders,
                   std::function<RenderPassViewData()> getCamInfo)
            : device_(device)
            , shaders_(shaders)
            , getCamInfoDataFunction_(std::move(getCamInfo))
        {
            // Shadow VS expects world matrices at b0 (not the standard kObjectSlot=b1,
            // because the GS uses b1 for the light constant buffer).
            objectSlot_.init(device_, 0);

            // Per-light layout: b1 light CB + t0 cascade StructuredBuffer (Vertex|Geometry visibility).
            nvrhi::BindingLayoutDesc lld;
            lld.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Geometry);
            lld.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(1));
            lld.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0));
            lightLayout_ = device_->createBindingLayout(lld);

            vs_ = shaders_.getOrCreate(device_, CascadeShadowShader, "VSMain", nvrhi::ShaderType::Vertex);
            gs_ = shaders_.getOrCreate(device_, CascadeShadowGeomShader, "GSMain", nvrhi::ShaderType::Geometry);
            inputLayout_ = rhi::createInputLayout(device_, rhi::makeVertexAttributes<VertexPNTBT>(), vs_);
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            if (!ctx.cmd) return;

            auto cam_info = getCamInfoDataFunction_();

            // Shadow casters: PNTBT opaque/masked DefaultLit solid geometry in the camera frustum.
            auto casters = SceneVisibility::ExtractFromFrustum(filter_objects_, cam_info.camera.GetViewProj());
            if (casters.empty()) return;

            // Directional lights: all of them (no frustum cull — shadows extend beyond the view).
            std::unordered_map<ObjectMask, DrawPacket> lights;
            SceneVisibility::ExpandByFilterFromAll(filter_lights_, lights);
            if (lights.empty()) return;

            for (auto& [mask, packet] : lights)
            {
                for (auto& [key, group] : packet.common_resource_renderables)
                {
                    for (auto& weak : group.renderables)
                    {
                        auto r = weak.lock();
                        if (!r) continue;

                        auto dirLight = std::dynamic_pointer_cast<DirectionalLightComponent>(r);
                        if (!dirLight) continue;
                        if (!dirLight->getShadowFramebuffer() || !dirLight->getCascadeBuffer())
                            continue;

                        // Recompute cascade matrices from the current camera and upload.
                        dirLight->updateCascadeGPUData(device_, ctx.cmd, cam_info.camera);

                        nvrhi::GraphicsPipelineHandle pipe = ensurePipeline(dirLight->getShadowFramebuffer());
                        if (!pipe) continue;

                        nvrhi::BindingSetHandle lightSet = ensureLightSet(dirLight.get());
                        if (!lightSet) continue;

                        uint32_t shadowSize = dirLight->getShadowSize();
                        auto vp = rhi::fullViewportState(shadowSize, shadowSize);

                        ctx.cmd->clearDepthStencilTexture(dirLight->getShadowTexture(),
                                                          nvrhi::AllSubresources,
                                                          /*clearDepth=*/true, 1.f,
                                                          /*clearStencil=*/false, 0);

                        for (auto& [cmask, cpacket] : casters)
                        {
                            for (auto& [ckey, cgroup] : cpacket.common_resource_renderables)
                            {
                                for (auto& cweak : cgroup.renderables)
                                {
                                    auto caster = cweak.lock();
                                    if (!caster) continue;

                                    nvrhi::GraphicsState state;
                                    state.setPipeline(pipe)
                                         .setFramebuffer(dirLight->getShadowFramebuffer())
                                         .setViewport(vp);
                                    state.addBindingSet(objectSlot_.set);
                                    state.addBindingSet(lightSet);
                                    rhi::drawRenderable(ctx.cmd, state, objectSlot_, *caster);
                                }
                            }
                        }
                    }
                }
            }
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

        nvrhi::IDevice* device_ = nullptr;
        rhi::ShaderCache& shaders_;
        std::function<RenderPassViewData()> getCamInfoDataFunction_;

        rhi::ObjectConstantSlot objectSlot_;
        nvrhi::BindingLayoutHandle lightLayout_;

        nvrhi::ShaderHandle vs_;
        nvrhi::ShaderHandle gs_;
        nvrhi::InputLayoutHandle inputLayout_;
        nvrhi::GraphicsPipelineHandle pipeline_;       // lazily created on first light
        nvrhi::IFramebuffer* pipelineFb_ = nullptr;    // framebuffer the pipeline was built against

        std::unordered_map<DirectionalLightComponent*, nvrhi::BindingSetHandle> lightSetCache_;

        int DepthBias = -15000;
        float DepthBiasClamp = 0;
        float SlopeScaledDepthBias = 8.000f;

        nvrhi::GraphicsPipelineHandle ensurePipeline(nvrhi::IFramebuffer* fb)
        {
            if (pipeline_ && pipelineFb_ == fb) return pipeline_;
            if (!vs_ || !gs_ || !inputLayout_ || !fb) return nullptr;

            nvrhi::RenderState rs;
            rs.rasterState.setFillSolid()
                          .setCullNone()
                          .setDepthBias(DepthBias)
                          .setDepthBiasClamp(DepthBiasClamp)
                          .setSlopeScaleDepthBias(SlopeScaledDepthBias)
                          .enableDepthClip();

            nvrhi::DepthStencilState ds;
            ds.enableDepthTest().enableDepthWrite().setDepthFunc(nvrhi::ComparisonFunc::Less);
            rs.setDepthStencilState(ds);

            nvrhi::GraphicsPipelineDesc pd;
            pd.setVertexShader(vs_)
              .setGeometryShader(gs_)
              .setInputLayout(inputLayout_)
              .setRenderState(rs);
            pd.addBindingLayout(objectSlot_.layout).addBindingLayout(lightLayout_);

            pipeline_ = device_->createGraphicsPipeline(pd, fb);
            pipelineFb_ = fb;
            return pipeline_;
        }

        nvrhi::BindingSetHandle ensureLightSet(DirectionalLightComponent* light)
        {
            auto it = lightSetCache_.find(light);
            if (it != lightSetCache_.end()) return it->second;

            nvrhi::BindingSetDesc sd;
            if (auto* cb = light->getLightCB())
                sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, cb));
            if (auto* buf = light->getCascadeBuffer())
                sd.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(0, buf));

            auto set = device_->createBindingSet(sd, lightLayout_);
            lightSetCache_[light] = set;
            return set;
        }
    };
}
