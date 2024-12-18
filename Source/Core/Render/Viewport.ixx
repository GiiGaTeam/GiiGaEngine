module;

#include<vector>
#include<d3d12.h>
#include<memory>
#include<directxtk/SimpleMath.h>


export module Viewport;

import RenderDevice;
export import GPULocalResource;
export import DescriptorHeap;
import BufferView;
import CameraComponent;
import RenderContext;


namespace GiiGa
{
    export class RenderGraph;

    export class Viewport
    {
    public:
        explicit Viewport(RenderDevice& device, std::shared_ptr<CameraComponent> camera = nullptr):
            device_(device), camera_(camera)
        {
        }

        virtual void Init() = 0;

        virtual ~Viewport() = default;

        virtual DescriptorHeapAllocation GetCameraDescriptor() =0;

        std::weak_ptr<CameraComponent> GetCameraComponent() const { return camera_; }

        void SetCameraComponent(const std::weak_ptr<CameraComponent>& camera)
        {
            if (camera_.expired()) return;
            camera_ = camera;
        }

        virtual void Execute(RenderContext& context) =0;

        void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            viewport_size_ = new_size;
            {
                D3D12_RESOURCE_DESC rtvDesc = {};
                rtvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                rtvDesc.Alignment = 0;
                rtvDesc.Width = static_cast<UINT>(viewport_size_.x);  // Set the width of the render target
                rtvDesc.Height = static_cast<UINT>(viewport_size_.y); // Set the height of the render target
                rtvDesc.DepthOrArraySize = 1;
                rtvDesc.MipLevels = 1;
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use a common format for render targets
                rtvDesc.SampleDesc.Count = 1;                // No multisampling
                rtvDesc.SampleDesc.Quality = 0;
                rtvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

                D3D12_CLEAR_VALUE clear_value = {};
                clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                clear_value.Color[0] = 1.0f;
                clear_value.Color[1] = 0.0f;
                clear_value.Color[2] = 0.0f;
                clear_value.Color[3] = 1.0f;

                resultResource_ = std::make_unique<GPULocalResource>(device_, rtvDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear_value);
            }

            {
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Specify the format of the render target
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;   // Use the first mip level
                rtvDesc.Texture2D.PlaneSlice = 0; // Only relevant for certain texture formats, usually set to 0

                resultRTV_ = resultResource_->CreateRenderTargetView(&rtvDesc);
            }

            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Specify the format of the SRV
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MostDetailedMip = 0;        // Use the most detailed mip level
                srvDesc.Texture2D.MipLevels = 1;              // Use one mip level
                srvDesc.Texture2D.PlaneSlice = 0;             // Only relevant for certain texture formats, usually set to 0
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // Resource minimum level-of-detail clamp

                // Create the Shader Resource View
                resultSRV_ = resultResource_->CreateShaderResourceBufferView(srvDesc);
            }
        }

    protected:
        RenderDevice& device_;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<RenderGraph> renderGraph_;
        std::unique_ptr<GPULocalResource> resultResource_;
        std::shared_ptr<BufferView<RenderTarget>> resultRTV_;
        std::shared_ptr<BufferView<ShaderResource>> resultSRV_;
        std::weak_ptr<CameraComponent> camera_;
    };
}
