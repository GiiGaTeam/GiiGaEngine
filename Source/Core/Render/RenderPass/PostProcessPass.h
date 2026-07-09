#pragma once

#include<memory>
#include<functional>

#include<nvrhi/nvrhi.h>

#include<RenderPass.h>
#include<Pipeline.h>
#include<RenderPassViewData.h>
#include<ShaderManager.h>
#include<RenderContext.h>
#include<GBuffer.h>

namespace GiiGa
{
    // Final gamma-correction pass: draws a full-screen triangle that samples the GBuffer's
    // LightAccumulation and writes the viewport's result texture. The full-quad VS is driven
    // by SV_VertexID, so there is no vertex/index buffer.
    class PostProcessPass : public RenderPass
    {
    public:
        PostProcessPass(RenderContext& context, const std::function<RTVData()>& getResultInfoFn, const std::shared_ptr<GBuffer>& gBuffer) :
            getResultInfoFunction_(getResultInfoFn), gBuffer_(gBuffer)
        {
            auto& device = context.GetDevice();

            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.setVisibility(nvrhi::ShaderType::All);
            layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0)); // LightAccum
            layoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            bindingLayout_ = device.Device()->createBindingLayout(layoutDesc);

            sampler_ = device.Device()->createSampler(
                nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(nvrhi::SamplerAddressMode::Wrap));

            Pipeline p;
            p.VS(ShaderManager::GetShaderByName(device, VertexFullQuadShader))
             .PS(ShaderManager::GetShaderByName(device, GammaCorrectionShader))
             .Raster(DefaultRasterState().setCullFront().setDepthClipEnable(false))
             .Depth(nvrhi::DepthStencilState().disableDepthTest().disableDepthWrite().disableStencil())
             .AddBindingLayout(bindingLayout_);

            pipeline_ = p.Build(device, getResultInfoFn().framebuffer_);
        }

        void Draw(RenderContext& context) override
        {
            auto result_info = getResultInfoFunction_();
            auto* cl = context.GetCommandList();
            auto& device = context.GetDevice();

            const auto& desc = result_info.resource_->getDesc();

            nvrhi::BindingSetDesc bsd;
            bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gBuffer_->GetColorTexture(GBuffer::GBufferOrder::LightAccumulation)));
            bsd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            nvrhi::BindingSetHandle set = device.Device()->createBindingSet(bsd, bindingLayout_);

            nvrhi::ViewportState vp;
            vp.addViewportAndScissorRect(nvrhi::Viewport(static_cast<float>(desc.width), static_cast<float>(desc.height)));

            nvrhi::GraphicsState state;
            state.setPipeline(pipeline_).setFramebuffer(result_info.framebuffer_).setViewport(vp);
            state.addBindingSet(set);

            cl->setGraphicsState(state);
            cl->draw(nvrhi::DrawArguments().setVertexCount(3));
        }

    private:
        nvrhi::GraphicsPipelineHandle pipeline_;
        nvrhi::BindingLayoutHandle bindingLayout_;
        nvrhi::SamplerHandle sampler_;
        std::shared_ptr<GBuffer> gBuffer_;
        std::function<RTVData()> getResultInfoFunction_;
    };
}
