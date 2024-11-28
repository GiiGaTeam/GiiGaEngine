module;

#include <memory>
#include <span>
#include <directx/d3dx12.h>
#include <directxtk/SimpleMath.h>

export module PerObjectData;
import TransformComponent;
import BufferView;
import GPULocalResource;
import RenderDevice;
import RenderContext;

namespace GiiGa
{

    struct WVPMatrices
    {
        DirectX::XMMATRIX World;
        DirectX::XMMATRIX WorldInverse;
    };
    export class PerObjectData
    {
        public:
        PerObjectData(RenderDevice Device, RenderContext& Context,
            std::shared_ptr<TransformComponent>& Transform, bool IsStatic) :
        IsStatic_(IsStatic),
        Transform_(Transform)
        {
            UINT SizeInBytes = static_cast<UINT>(sizeof(Matrix)) * 3;
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

            WVPMatrices WVPMatrices;
            WVPMatrices.World = Transform_->GetWorldMatrix();
            WVPMatrices.WorldInverse = Transform_->GetInverseWorldMatrix();
            const auto WVPMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WVPMatrices), SizeInBytes};
            
            GPULocalResource ConstantBuffer_ = GPULocalResource(Device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
            ConstantBuffer_.UpdateContents(Context, WVPMatricesSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            ConstantBufferView_ = ConstantBuffer_.CreateConstantBufferView(desc);
        }

        void UpdateGPUData(RenderContext& Context)
        {
            if (IsStatic_) return;
            
            UINT SizeInBytes = static_cast<UINT>(sizeof(Matrix)) * 3;
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
            
            WVPMatrices WVPMatrices;
            WVPMatrices.World = Transform_->GetWorldMatrix();
            WVPMatrices.WorldInverse = Transform_->GetInverseWorldMatrix();
            const auto WVPMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WVPMatrices), SizeInBytes};
            
            ConstantBufferView_ = Context.AllocateDynamicConstantView(WVPMatricesSpan, 0, desc);

        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptor() const
        {
            return ConstantBufferView_->getDescriptor().getGPUHandle();
        }
        
        private:
        bool IsStatic_;
        std::shared_ptr<TransformComponent> Transform_;
        std::shared_ptr<BufferView<Constant>> ConstantBufferView_;
    };
}