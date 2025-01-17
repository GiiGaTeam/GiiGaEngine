#pragma once
#include<directxtk12/SimpleMath.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>

#include<memory>
#include<array>

#include<GPULocalResource.h>
#include<RenderDevice.h>
#include<RenderContext.h>

namespace GiiGa
{
    /*
     *  GBuffer Structure:
     *                          R       G       B       A
     *      LightAccum          R       G       B    depth(copy)
     *      Diffuse             R       G       B      NU
     *      MatProps         Metal    Spec    Rough    Aniso
     *      NormalWS            X       Y       Z      NU
     *      PostionWS           X       Y       Z      NU  - Deprecated
     *      
     **** Depth/Stencil              D24_UNORM     S8_UINT
     */

    class GBuffer
    {
    public:
        enum class GBufferOrder
        {
            LightAccumulation,
            Diffuse,
            Material,
            NormalWS,
        };

        static inline const uint8_t NUM_GBUFFERS = 4;
        static inline const DXGI_FORMAT G_FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;
        static inline const D3D12_CLEAR_VALUE G_CLEAR_VALUE = {.Format = G_FORMAT, .Color = {0.0f, 0.0f, 0.0f, 1.0f}};
        static inline const DXGI_FORMAT DS_FORMAT_RES = DXGI_FORMAT_R24G8_TYPELESS;
        static inline const DXGI_FORMAT DS_FORMAT_DSV = DXGI_FORMAT_D24_UNORM_S8_UINT;
        static inline const D3D12_CLEAR_VALUE DS_CLEAR_VALUE = {.Format = DS_FORMAT_DSV, .DepthStencil = {.Depth = 1, .Stencil = 1}};

        GBuffer(RenderDevice& device, DirectX::SimpleMath::Vector2 size):
            device_(device)
        {
            Resize(size);
        }

        void ClearAll(RenderContext& context)
        {
            for (auto rtv : RTVs_)
            {
                context.ClearRenderTargetView(rtv->getDescriptor().GetCpuHandle(), G_CLEAR_VALUE);
            }
            context.ClearDepthStencilView(DSV_->getDescriptor().GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, DS_CLEAR_VALUE);
        }

        void TransitionResource(RenderContext& context, GBufferOrder buffer, D3D12_RESOURCE_STATES after) const
        {
            resources_[static_cast<int>(buffer)]->StateTransition(context, after);
        }

        void TransitionDepthResource(RenderContext& context, D3D12_RESOURCE_STATES after) const
        {
            depth_res_->StateTransition(context, after);
        }

        void BindAllAsRTV(RenderContext& context) const
        {
            std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NUM_GBUFFERS> rtvs;

            for (int i = 0; i < RTVs_.size(); ++i)
                rtvs[i] = RTVs_[i]->getDescriptor().GetCpuHandle();

            auto dsv = DSV_->getDescriptor().GetCpuHandle();

            context.GetGraphicsCommandList()->OMSetRenderTargets(NUM_GBUFFERS, rtvs.data(),
                                                                 false, &dsv);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(GBufferOrder buffer) const
        {
            return RTVs_[static_cast<int>(buffer)]->getDescriptor().GetCpuHandle();
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetSRV(GBufferOrder buffer) const
        {
            return SRVs_[static_cast<int>(buffer)]->getDescriptor().getGPUHandle();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthRTV() const
        {
            return DSV_->getDescriptor().GetCpuHandle();
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSRV() const
        {
            return SRVs_[NUM_GBUFFERS]->getDescriptor().getGPUHandle();
        }

        void ClearStencil(RenderContext& context, uint8_t value)
        {
            context.ClearDepthStencilView(DSV_->getDescriptor().GetCpuHandle(), D3D12_CLEAR_FLAG_STENCIL, D3D12_CLEAR_VALUE{.DepthStencil = {.Depth = 0, .Stencil = value}});
        }

        void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            for (uint32_t i = 0; i < NUM_GBUFFERS; ++i)
            {
                {
                    D3D12_RESOURCE_DESC rtvDesc = {};
                    rtvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                    rtvDesc.Alignment = 0;
                    rtvDesc.Width = static_cast<UINT>(new_size.x);  // Set the width of the render target
                    rtvDesc.Height = static_cast<UINT>(new_size.y); // Set the height of the render target
                    rtvDesc.DepthOrArraySize = 1;
                    rtvDesc.MipLevels = 1;
                    rtvDesc.Format = G_FORMAT;    // Use a common format for render targets
                    rtvDesc.SampleDesc.Count = 1; // No multisampling
                    rtvDesc.SampleDesc.Quality = 0;
                    rtvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                    rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

                    D3D12_CLEAR_VALUE clear_value = G_CLEAR_VALUE;

                    resources_[i] = std::make_unique<GPULocalResource>(device_, rtvDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear_value);
                }

                {
                    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                    rtvDesc.Format = G_FORMAT; // Specify the format of the render target
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = 0;   // Use the first mip level
                    rtvDesc.Texture2D.PlaneSlice = 0; // Only relevant for certain texture formats, usually set to 0

                    RTVs_[i] = resources_[i]->EmplaceRenderTargetView(&rtvDesc);
                }

                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = G_FORMAT; // Specify the format of the SRV
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srvDesc.Texture2D.MostDetailedMip = 0;        // Use the most detailed mip level
                    srvDesc.Texture2D.MipLevels = 1;              // Use one mip level
                    srvDesc.Texture2D.PlaneSlice = 0;             // Only relevant for certain texture formats, usually set to 0
                    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // Resource minimum level-of-detail clamp

                    // Create the Shader Resource View
                    SRVs_[i] = resources_[i]->EmplaceShaderResourceBufferView(srvDesc);
                }
            }

            {
                D3D12_RESOURCE_DESC res_desc = {};
                res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                res_desc.Alignment = 0;
                res_desc.Width = static_cast<UINT>(new_size.x);  // Set the width of the render target
                res_desc.Height = static_cast<UINT>(new_size.y); // Set the height of the render target
                res_desc.DepthOrArraySize = 1;
                res_desc.MipLevels = 1;
                res_desc.Format = DS_FORMAT_RES; // Use a common format for render targets
                res_desc.SampleDesc.Count = 1;   // No multisampling
                res_desc.SampleDesc.Quality = 0;
                res_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                D3D12_CLEAR_VALUE clear_value = DS_CLEAR_VALUE;

                depth_res_ = std::make_unique<GPULocalResource>(device_, res_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value);

                D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
                dsv_desc.Format = DS_FORMAT_DSV; // Use an appropriate format for the depth stencil view
                dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
                dsv_desc.Texture2D.MipSlice = 0; // Use the first mip level

                DSV_ = depth_res_->EmplaceDepthStencilView(dsv_desc);

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // Specify the format of the SRV
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MostDetailedMip = 0;        // Use the most detailed mip level
                srvDesc.Texture2D.MipLevels = 1;              // Use one mip level
                srvDesc.Texture2D.PlaneSlice = 0;             // Only relevant for certain texture formats, usually set to 0
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // Resource minimum level-of-detail clamp

                // Create the Shader Resource View
                SRVs_[NUM_GBUFFERS] = depth_res_->EmplaceShaderResourceBufferView(srvDesc);
            }
        }

    private:
        RenderDevice& device_;
        std::array<std::unique_ptr<GPULocalResource>, NUM_GBUFFERS> resources_;
        std::array<std::shared_ptr<BufferView<RenderTarget>>, NUM_GBUFFERS> RTVs_;
        std::array<std::shared_ptr<BufferView<ShaderResource>>, NUM_GBUFFERS + 1> SRVs_;
        std::unique_ptr<GPULocalResource> depth_res_;
        std::shared_ptr<BufferView<DepthStencil>> DSV_;
    };
}
