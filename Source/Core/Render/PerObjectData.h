#pragma once

#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<DirectXMath.h>

#include<nvrhi/nvrhi.h>

#include<TransformComponent.h>
#include<RenderDevice.h>
#include<RenderContext.h>

namespace GiiGa
{
    struct alignas(256) WorldMatrices
    {
        DirectX::XMMATRIX World;
        DirectX::XMMATRIX WorldInverse;
    };

    // Per-object constant buffer (world + inverse-world), bound at register b1.
    // Backed by an NVRHI volatile constant buffer, re-written every frame from the
    // per-frame UpdateGPUData pass while the command list is open.
    class PerObjectData
    {
    public:
        PerObjectData() = default;

        PerObjectData(RenderContext& Context,
                      std::shared_ptr<TransformComponent> Transform, bool IsStatic):
            IsStatic_(IsStatic),
            Transform_(Transform)
        {
            cb_ = Context.GetDevice().Device()->createBuffer(
                nvrhi::BufferDesc()
                .setByteSize(sizeof(WorldMatrices))
                .setIsConstantBuffer(true)
                .setIsVolatile(true)
                .setMaxVersions(64)
                .setDebugName("PerObjectCB"));
        }

        void UpdateGPUData(RenderContext& Context)
        {
            auto tf = Transform_.lock();
            if (!tf || !cb_) return;

            WorldMatrices wm;
            wm.World = tf->GetWorldMatrix().Transpose();
            wm.WorldInverse = tf->GetInverseWorldMatrix().Transpose();

            Context.GetCommandList()->writeBuffer(cb_, &wm, sizeof(wm));
        }

        nvrhi::IBuffer* GetBuffer() const { return cb_; }

    private:
        bool IsStatic_ = true;
        std::weak_ptr<TransformComponent> Transform_;
        nvrhi::BufferHandle cb_;
    };
}
