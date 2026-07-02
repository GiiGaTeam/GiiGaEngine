#pragma once

// NvrhiConstants.h
//
// Constant-buffer payload layouts for the NVRHI render path. These are the plain structs
// uploaded into volatile/static constant buffers (see NvrhiResources.h) and bound via
// nvrhi::BindingSet. They replace the alignas(256) structs embedded in the legacy
// PerObjectData.h and RenderPassViewData.h, which were tied to the hand-rolled descriptor
// heap machinery.

#include <directxtk12/SimpleMath.h>

namespace GiiGa::rhi
{
    // Per-object (model) constants — matches the `b1` register in the engine's HLSL.
    struct ObjectConstants
    {
        DirectX::SimpleMath::Matrix world;
        DirectX::SimpleMath::Matrix worldInverse;
    };

    // Per-view (camera) constants — matches the `b0` register.
    struct ViewConstants
    {
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projMatrix;
        DirectX::SimpleMath::Matrix invViewMatrix;
        DirectX::SimpleMath::Matrix invProjMatrix;
        DirectX::SimpleMath::Vector3 camPos;
        float pad0 = 0.f;
    };

    // Screen dimensions — matches the legacy ScreenDimensions `b3`-ish payload.
    struct ScreenDimensions
    {
        DirectX::SimpleMath::Vector2 dimensions;
        float pad0 = 0.f;
        float pad1 = 0.f;
    };
}
