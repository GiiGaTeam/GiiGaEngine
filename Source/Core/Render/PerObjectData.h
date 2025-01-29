#pragma once


#include<memory>
#include<span>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directx/d3dx12.h>
#include<directxtk12/SimpleMath.h>

#include<TransformComponent.h>
#include<BufferView.h>
#include<GPULocalResource.h>
#include<RenderDevice.h>
#include<RenderContext.h>

#include "Mesh.h"
#include "Skeleton.h"
#include "ozz-animation/PlaybackController.h"

namespace GiiGa
{
    struct alignas(256) WorldMatrices
    {
        DirectX::XMMATRIX World;
        DirectX::XMMATRIX WorldInverse;
    };

    class PerObjectData
    {
    public:
        PerObjectData() = default;

        PerObjectData(RenderContext& Context,
                      std::shared_ptr<TransformComponent> Transform, bool IsStatic) :
            IsStatic_(IsStatic),
            Transform_(Transform)
        {
            UINT SizeInBytes = sizeof(WorldMatrices);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

            WorldMatrices WorldMatrices;
            WorldMatrices.World = Transform_->GetWorldMatrix().Transpose();
            WorldMatrices.WorldInverse = Transform_->GetInverseWorldMatrix().Transpose();
            const auto WorldMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WorldMatrices), SizeInBytes};

            ConstantBuffer_ = std::make_unique<GPULocalResource>(Context.GetDevice(), CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
            ConstantBuffer_->UpdateContentsDeffered(Context, WorldMatricesSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            ConstantBufferView_ = ConstantBuffer_->CreateConstantBufferView(desc);
        }

        virtual void UpdateGPUData(RenderContext& Context)
        {
            UINT SizeInBytes = sizeof(WorldMatrices);

            WorldMatrices WorldMatrices;
            WorldMatrices.World = Transform_->GetWorldMatrix().Transpose();
            WorldMatrices.WorldInverse = Transform_->GetInverseWorldMatrix().Transpose();
            const auto WorldMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WorldMatrices), SizeInBytes};

            if (IsStatic_)
            {
                ConstantBuffer_->UpdateContentsImmediate(Context, WorldMatricesSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

                ConstantBufferView_ = Context.AllocateDynamicConstantView(WorldMatricesSpan, desc);
            }
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptor() const
        {
            return ConstantBufferView_->getDescriptor().getGPUHandle();
        }

    protected:
        bool IsStatic_;
        std::shared_ptr<GPULocalResource> ConstantBuffer_;
        std::shared_ptr<TransformComponent> Transform_;
        std::shared_ptr<BufferView<Constant>> ConstantBufferView_;
    };

    class PerAnimatedObjectData : public PerObjectData
    {
        struct alignas(256) SkinData
        {
            DirectX::XMMATRIX matricies[96];
        };
        public:
        PerAnimatedObjectData() = default;
        PerAnimatedObjectData(RenderContext& Context,
            const std::shared_ptr<TransformComponent>& Transform, const std::shared_ptr<Skeleton>& Skeleton,
            const std::shared_ptr<ozz::sample::Mesh>& skeletal_mesh) :
            PerObjectData(Context, Transform, false), Skeleton_(Skeleton), skeletal_mesh_(skeletal_mesh)
        {
            UINT SizeInBytes = sizeof(SkinData);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

            SkinData skindata_{};
            const auto SkinDataSpan = std::span{reinterpret_cast<uint8_t*>(&skindata_), SizeInBytes};
            
            ConstantBuffer_ = std::make_unique<GPULocalResource>(Context.GetDevice(), CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
            ConstantBuffer_->UpdateContentsDeffered(Context, SkinDataSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            ConstantBufferView_ = ConstantBuffer_->CreateConstantBufferView(desc);
        }

        void UpdateGPUData(RenderContext& Context) override
        {
            PerObjectData::UpdateGPUData(Context);
            
            UINT SizeInBytes = sizeof(SkinData);

            SkinData skindata_{};
            
            WorldMatrices.World = Transform_->GetWorldMatrix().Transpose();
            WorldMatrices.WorldInverse = Transform_->GetInverseWorldMatrix().Transpose();
            const auto WorldMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&WorldMatrices), SizeInBytes};

            if (IsStatic_)
            {
                ConstantBuffer_->UpdateContentsImmediate(Context, WorldMatricesSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

                ConstantBufferView_ = Context.AllocateDynamicConstantView(WorldMatricesSpan, desc);
            }
        }
        private:
        std::shared_ptr<Skeleton> Skeleton_;
        std::shared_ptr<ozz::sample::Mesh> skeletal_mesh_;
        std::shared_ptr<GPULocalResource> SkinningConstantBuffer_;
        std::shared_ptr<BufferView<Constant>> SkinningConstantBufferView_;
    };
}
