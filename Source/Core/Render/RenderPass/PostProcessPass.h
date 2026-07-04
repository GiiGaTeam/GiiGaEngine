#pragma once

// PostProcessPass.h — final post-processing pass (NVRHI).
//
// Draws a fullscreen quad (DefaultAssetsHandles::Quad) sampling the GBuffer LightAccumulation
// SRV through a gamma-correction pixel shader into the viewport's result texture.
// Bindings: t0 = LightAccumulation SRV, s0 = linear-wrap sampler. No view/object constants.
// Shaders: VertexFullQuadShader(VSMain) + GammaCorrectionShader(PSMain). Depth disabled.

#include <memory>
#include <functional>

#include <Core/Render/Nvrhi/NvrhiRenderPass.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiShader.h>
#include <Core/Render/Nvrhi/NvrhiResources.h>
#include <Core/Render/Nvrhi/NvrhiPassCommon.h>

#include <RenderPassViewData.h>
#include <DefaultAssetsHandles.h>
#include <Engine.h>
#include <ConcreteAsset/MeshAsset.h>

namespace GiiGa
{
    class PostProcessPass : public rhi::RenderPass
    {
    public:
        PostProcessPass(nvrhi::IDevice* device, rhi::ShaderCache& shaders,
                        std::function<RTVData()> getResultInfo,
                        std::shared_ptr<rhi::GBuffer> gbuffer)
            : device_(device)
            , shaders_(shaders)
            , getResultInfoFunction_(std::move(getResultInfo))
            , gbuffer_(std::move(gbuffer))
        {
            sampler_ = rhi::getLinearWrapSampler(device_);

            nvrhi::BindingLayoutDesc bld;
            bld.setVisibility(nvrhi::ShaderType::Pixel);
            bld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
            bld.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            bindingLayout_ = device_->createBindingLayout(bld);

            nvrhi::InputLayoutHandle inputLayout = rhi::createInputLayout(
                device_, rhi::makeVertexAttributes<VertexPNTBT>(),
                shaders_.getOrCreate(device_, VertexFullQuadShader, "VSMain", nvrhi::ShaderType::Vertex));

            nvrhi::RenderState rs;
            rs.rasterState.setCullFront().setFrontCounterClockwise(true).disableDepthClip();
            rs.depthStencilState.disableDepthTest();

            RTVData initial = getResultInfoFunction_();
            if (initial.resource)
            {
                nvrhi::FramebufferDesc fbd;
                fbd.addColorAttachment(initial.resource);
                resultFb_ = device_->createFramebuffer(fbd);
                resultFbSource_ = initial.resource.Get();
            }

            nvrhi::GraphicsPipelineDesc pd;
            pd.setVertexShader(shaders_.getOrCreate(device_, VertexFullQuadShader, "VSMain", nvrhi::ShaderType::Vertex))
              .setPixelShader(shaders_.getOrCreate(device_, GammaCorrectionShader, "PSMain", nvrhi::ShaderType::Pixel))
              .setInputLayout(inputLayout)
              .setRenderState(rs)
              .addBindingLayout(bindingLayout_);
            pipeline_ = device_->createGraphicsPipeline(pd, resultFb_.Get());

            auto rm = Engine::Instance().ResourceManager();
            mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(DefaultAssetsHandles::Quad);
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            if (!ctx.cmd || !gbuffer_ || !pipeline_ || !mesh_) return;

            RTVData result = getResultInfoFunction_();
            if (!result.resource) return;

            if (resultFbSource_ != result.resource.Get())
            {
                nvrhi::FramebufferDesc fbd;
                fbd.addColorAttachment(result.resource);
                resultFb_ = device_->createFramebuffer(fbd);
                resultFbSource_ = result.resource.Get();
            }

            nvrhi::BindingSetDesc bsd;
            bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gbuffer_->getColor(rhi::GBuffer::Order::LightAccumulation)));
            bsd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            nvrhi::BindingSetHandle set = device_->createBindingSet(bsd, bindingLayout_);

            nvrhi::GraphicsState state;
            state.setPipeline(pipeline_).setFramebuffer(resultFb_.Get());
            state.setViewport(rhi::fullViewportState(gbuffer_->width(), gbuffer_->height()));
            state.addBindingSet(set);

            nvrhi::VertexBufferBinding vbb;
            vbb.buffer = mesh_->GetVertexBuffer();
            vbb.slot = 0;
            vbb.offset = 0;
            state.addVertexBuffer(vbb);

            nvrhi::IndexBufferBinding ibb;
            ibb.buffer = mesh_->GetIndexBuffer();
            ibb.format = mesh_->GetIndexFormat();
            ibb.offset = 0;
            state.setIndexBuffer(ibb);

            ctx.cmd->setGraphicsState(state);
            ctx.cmd->drawIndexed(nvrhi::DrawArguments().setVertexCount(mesh_->GetIndexCount()));
        }

    private:
        nvrhi::IDevice* device_ = nullptr;
        rhi::ShaderCache& shaders_;
        std::function<RTVData()> getResultInfoFunction_;
        std::shared_ptr<rhi::GBuffer> gbuffer_;

        nvrhi::BindingLayoutHandle bindingLayout_;
        nvrhi::SamplerHandle sampler_;

        nvrhi::FramebufferHandle resultFb_;
        nvrhi::ITexture* resultFbSource_ = nullptr;

        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        nvrhi::GraphicsPipelineHandle pipeline_;
    };
}
