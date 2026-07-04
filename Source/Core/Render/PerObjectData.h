#pragma once

// PerObjectData.h — per-object (model) constant source.
// NVRHI cutover: no longer owns a D3D12 constant buffer / descriptor view. It just captures
// the transform and fills an ObjectConstants struct; the owning pass writes that into a
// volatile constant buffer once per draw (the idiomatic NVRHI pattern for per-object data).

#include <memory>
#include <Core/Render/Nvrhi/NvrhiConstants.h>
#include <TransformComponent.h>

namespace GiiGa
{
    class PerObjectData
    {
    public:
        PerObjectData() = default;

        explicit PerObjectData(std::shared_ptr<TransformComponent> transform)
            : transform_(transform)
        {
        }

        void SetTransform(std::shared_ptr<TransformComponent> transform)
        {
            transform_ = transform;
        }

        // Fill per-object world matrices (transposed to match the engine's HLSL row-major convention).
        void Fill(rhi::ObjectConstants& out) const
        {
            if (auto t = transform_.lock())
            {
                out.world = t->GetWorldMatrix().Transpose();
                out.worldInverse = t->GetInverseWorldMatrix().Transpose();
            }
        }

    private:
        std::weak_ptr<TransformComponent> transform_;
    };
}
