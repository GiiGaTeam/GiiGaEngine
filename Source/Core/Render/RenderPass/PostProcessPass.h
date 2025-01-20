#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#include "GBuffer.h"
#include "Material.h"
#endif
#include<d3d12.h>
#include<directx/d3dx12_core.h>


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

namespace GiiGa
{
    class PostProcessPass : public RenderPass
    {
    public:
        PostProcessPass(RenderContext& context, const std::function<RTVData()>& getResultInfoFn, const std::shared_ptr<GBuffer>& gBuffer) :
            getResultInfoFunction_(getResultInfoFn), gBuffer_(gBuffer)
        {
            D3D12_RASTERIZER_DESC rast_desc = {
                .FillMode = D3D12_FILL_MODE_SOLID,
                .CullMode = D3D12_CULL_MODE_FRONT,
                .FrontCounterClockwise = TRUE,
                .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
                .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
                .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
                .DepthClipEnable = FALSE,
                .MultisampleEnable = FALSE,
                .AntialiasedLineEnable = FALSE,
                .ForcedSampleCount = 0,
                .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
            };

            auto sampler_desc = D3D12_STATIC_SAMPLER_DESC{
                .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                .MinLOD = 0,
                .MaxLOD = D3D12_FLOAT32_MAX
            };

            D3D12_DEPTH_STENCIL_DESC depth_state_desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            depth_state_desc.DepthEnable = false;
            depth_state_desc.StencilEnable = false;
            

            DXGI_FORMAT rtv_format_array[] = {GBuffer::G_FORMAT};

            gamma_correction_pso
                .set_vs(ShaderManager::GetShaderByName(VertexFullQuadShader))
                .set_ps(ShaderManager::GetShaderByName(GammaCorrectionShader))
                .set_rasterizer_state(rast_desc)
                .set_input_layout(VertexPNTBT::InputLayout)
                .set_rtv_format(rtv_format_array, 1)
                .set_dsv_format(DXGI_FORMAT_UNKNOWN)
                .set_depth_stencil_state(depth_state_desc)
                .SetPerObjectDataFunction([](RenderContext& context, PerObjectData& per_obj)
                {
                    //context.BindDescriptorHandle(ModelDataRootIndex, per_obj.GetDescriptor());
                })
                .SetShaderResourceFunction([](RenderContext& context, IObjectShaderResource& resource)
                {
                    /*const auto& descs = resource.GetDescriptors();
                    context.BindDescriptorHandle(MaterialDataRootIndex, descs[0]);*/
                })
                .add_static_samplers(sampler_desc)
                .GeneratePSO(context.GetDevice(), 0, 1);

            auto rm = Engine::Instance().ResourceManager();

            mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(DefaultAssetsHandles::Quad);
        }

        void Draw(RenderContext& context) override
        {
            auto result_info = getResultInfoFunction_();

            context.BindPSO(gamma_correction_pso.GetState().get());
            context.SetSignature(gamma_correction_pso.GetSignature().get());

            gBuffer_->TransitionResource(context, GBuffer::GBufferOrder::LightAccumulation, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            result_info.resource_->StateTransition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);
            context.BindDescriptorHandle(0, gBuffer_->GetSRV(GBuffer::GBufferOrder::LightAccumulation));

            const auto rtv = result_info.RTV_->getDescriptor().GetCpuHandle();
            const auto dsv = gBuffer_->GetDepthRTV();
            context.GetGraphicsCommandList()->OMSetRenderTargets(1, &rtv, false, nullptr);
            mesh_->Draw(context.GetGraphicsCommandList());
            //context.GetGraphicsCommandList()->IASetVertexBuffers(0, 0, nullptr);
            //context.GetGraphicsCommandList()->DrawInstanced(6, 1, 0, 0);
        }

    private:
        PSO gamma_correction_pso;
        std::shared_ptr<GBuffer> gBuffer_;
        std::function<RTVData()> getResultInfoFunction_;

        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
    };
}
