#pragma once


#include<memory>
#include<span>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<nvrhi/nvrhi.h>

#include<TransformComponent.h>
#include<NvrhiUtils.h>
#include<RenderDevice.h>
#include<RenderContext.h>

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
            ConstantBuffer_ = CreateConstantBuffer(Context.GetDevice().NvDevice(), sizeof(WorldMatrices), "PerObjectData");

            WorldMatrices matrices;
            matrices.World = Transform_.lock()->GetWorldMatrix().Transpose();
            matrices.WorldInverse = Transform_.lock()->GetInverseWorldMatrix().Transpose();
            const auto span = std::span{reinterpret_cast<const uint8_t*>(&matrices), sizeof(WorldMatrices)};

            Context.WriteBufferDeffered(ConstantBuffer_, span);
        }

        void UpdateGPUData(RenderContext& Context)
        {
            WorldMatrices matrices;
            matrices.World = Transform_.lock()->GetWorldMatrix().Transpose();
            matrices.WorldInverse = Transform_.lock()->GetInverseWorldMatrix().Transpose();
            const auto span = std::span{reinterpret_cast<const uint8_t*>(&matrices), sizeof(WorldMatrices)};

            Context.WriteBufferImmediate(ConstantBuffer_, span);
        }

        nvrhi::IBuffer* GetConstantBuffer() const
        {
            return ConstantBuffer_;
        }

    private:
        bool IsStatic_;
        std::weak_ptr<TransformComponent> Transform_;
        nvrhi::BufferHandle ConstantBuffer_;
    };
}
