#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>


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
#include<GBuffer.h>
#include<Material.h>

namespace GiiGa
{
    class PostProcessPass : public RenderPass
    {
    public:
        PostProcessPass(RenderContext& context, const std::function<RTVData()>& getResultInfoFn, const std::shared_ptr<GBuffer>& gBuffer) :
            getResultInfoFunction_(getResultInfoFn), gBuffer_(gBuffer)
        {
            nvrhi::RasterState rast_state;
            rast_state.fillMode = nvrhi::RasterFillMode::Solid;
            rast_state.cullMode = nvrhi::RasterCullMode::Front;
            rast_state.frontCounterClockwise = true;
            rast_state.depthClipEnable = false;

            nvrhi::SamplerDesc sampler_desc;
            sampler_desc.minFilter = sampler_desc.magFilter = sampler_desc.mipFilter = true;
            sampler_desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);

            nvrhi::DepthStencilState depth_state;
            depth_state.depthTestEnable = false;
            depth_state.depthWriteEnable = false;
            depth_state.stencilEnable = false;

            gamma_correction_pso
                .set_vs(ShaderManager::GetShaderByName(VertexFullQuadShader))
                .set_ps(ShaderManager::GetShaderByName(GammaCorrectionShader))
                .set_rasterizer_state(rast_state)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_depth_stencil_state(depth_state)
                .add_texture_srv(0) // LightAccum
                .add_sampler(0, sampler_desc)
                .GeneratePSO(context.GetDevice());

            auto rm = Engine::Instance().ResourceManager();

            mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(DefaultAssetsHandles::Quad);
        }

        void Draw(RenderContext& context) override
        {
            auto result_info = getResultInfoFunction_();

            nvrhi::BindingSetDesc binding_desc;
            binding_desc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, gBuffer_->GetTexture(GBuffer::GBufferOrder::LightAccumulation)));
            gamma_correction_pso.AppendSamplers(binding_desc);

            context.SetFramebuffer(result_info.framebuffer);
            context.SetPipeline(gamma_correction_pso.GetOrCreatePipeline(context.GetDevice(), result_info.framebuffer));
            context.SetBindingSet(context.GetOrCreateBindingSet(binding_desc, gamma_correction_pso.GetBindingLayout()));

            mesh_->Draw(context);
        }

    private:
        PSO gamma_correction_pso;
        std::shared_ptr<GBuffer> gBuffer_;
        std::function<RTVData()> getResultInfoFunction_;

        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
    };
}
