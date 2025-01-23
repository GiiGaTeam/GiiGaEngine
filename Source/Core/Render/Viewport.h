#pragma once


#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>

#include<RenderDevice.h>
#include<RenderPassViewData.h>
#include<GBuffer.h>

namespace GiiGa
{
    class RenderGraph;

    class Viewport
    {
    public:
        explicit Viewport(RenderDevice& device):
            device_(device),
            gbuffer_(std::make_shared<GBuffer>(device, viewport_size_))
        {
        }

        virtual void Init(RenderContext& context)
        {
            CreateRenderTargetResult();
        }

        virtual ~Viewport() = default;

        virtual RenderPassViewData GetCameraInfo() =0;

        virtual RTVData GetResultInfo() const { return rtvData_; };

        virtual void Execute(RenderContext& context) =0;

        virtual void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            viewport_size_ = new_size;

            gbuffer_->Resize(new_size);

            CreateRenderTargetResult();
        }

        virtual void SetCamera(std::shared_ptr<CameraComponent> camera) =0;

        virtual void CreateRenderTargetResult()
        {
            {
                D3D12_RESOURCE_DESC rtvDesc = {};
                rtvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                rtvDesc.Alignment = 0;
                rtvDesc.Width = static_cast<UINT>(viewport_size_.x);  // Set the width of the render target
                rtvDesc.Height = static_cast<UINT>(viewport_size_.y); // Set the height of the render target
                rtvDesc.DepthOrArraySize = 1;
                rtvDesc.MipLevels = 1;
                rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Use a common format for render targets
                rtvDesc.SampleDesc.Count = 1;                    // No multisampling
                rtvDesc.SampleDesc.Quality = 0;
                rtvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

                D3D12_CLEAR_VALUE clear_value = {.Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .Color = {0.0f, 0.0f, 0.0f, 1.0f}};

                rtvData_.resource_ = std::make_shared<GPULocalResource>(device_, rtvDesc, D3D12_RESOURCE_STATE_COMMON, &clear_value);
            }

            {
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Specify the format of the render target
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;   // Use the first mip level
                rtvDesc.Texture2D.PlaneSlice = 0; // Only relevant for certain texture formats, usually set to 0

                rtvData_.RTV_ = rtvData_.resource_->EmplaceRenderTargetView(&rtvDesc);
            }

            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Specify the format of the SRV_
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MostDetailedMip = 0;        // Use the most detailed mip level
                srvDesc.Texture2D.MipLevels = 1;              // Use one mip level
                srvDesc.Texture2D.PlaneSlice = 0;             // Only relevant for certain texture formats, usually set to 0
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // Resource minimum level-of-detail clamp

                // Create the Shader Resource View
                rtvData_.SRV_ = rtvData_.resource_->EmplaceShaderResourceBufferView(srvDesc);
            }
        }

    protected:
        RenderDevice& device_;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<RenderGraph> renderGraph_;
        std::shared_ptr<GBuffer> gbuffer_;

        RTVData rtvData_;
    };
}
