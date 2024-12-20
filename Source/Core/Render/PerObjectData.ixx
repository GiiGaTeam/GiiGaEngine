export module PerObjectData;

import <memory>;
import <span>;
import <directx/d3dx12.h>;
import <directxtk/SimpleMath.h>;

import TransformComponent;
import BufferView;
import GPULocalResource;
import RenderDevice;
import RenderContext;

namespace GiiGa
{

    struct WorldMatrices
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
            UINT SizeInBytes = sizeof(WorldMatrices);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

            WorldMatrices WorldMatrices;
            WorldMatrices.World = Transform_->GetWorldMatrix();
            WorldMatrices.WorldInverse = Transform_->GetInverseWorldMatrix();
            const auto WorldMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WorldMatrices), SizeInBytes};
            
            ConstantBuffer_ = std::make_unique<GPULocalResource>(Device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
            ConstantBuffer_->UpdateContentsDeffered(Context, WorldMatricesSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            ConstantBufferView_ = ConstantBuffer_->CreateConstantBufferView(desc);
        }

        void UpdateGPUData(RenderContext& Context)
        {
            UINT SizeInBytes = sizeof(WorldMatrices);
                
            WorldMatrices WorldMatrices;
            WorldMatrices.World = Transform_->GetWorldMatrix();
            WorldMatrices.WorldInverse = Transform_->GetInverseWorldMatrix();
            const auto WorldMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WorldMatrices), SizeInBytes};
            
            if (IsStatic_)
            {
                ConstantBuffer_->UpdateContentsImmediate(Context, WorldMatricesSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
            
                ConstantBufferView_ = Context.AllocateDynamicConstantView(WorldMatricesSpan, 0, desc);
            }

        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptor() const
        {
            return ConstantBufferView_->getDescriptor().getGPUHandle();
        }
        
        private:
        bool IsStatic_;
        std::shared_ptr<GPULocalResource> ConstantBuffer_;
        std::shared_ptr<TransformComponent> Transform_;
        std::shared_ptr<BufferView<Constant>> ConstantBufferView_;
    };
}