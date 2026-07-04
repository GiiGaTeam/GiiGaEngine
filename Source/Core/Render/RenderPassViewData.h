#pragma once

// RenderPassViewData.h — per-view data carried through the render graph.
// NVRHI cutover: dropped the D3D12_GPU_DESCRIPTOR_HANDLE fields (the pass now owns the view
// constant buffer + binding set). This struct carries only CPU-side data: the camera (for
// culling math) and the uploaded ViewConstants / ScreenDimensions payloads.

#include <directxtk12/SimpleMath.h>
#include <Core/Render/Nvrhi/NvrhiConstants.h>
#include <CameraComponent.h>

namespace GiiGa
{
    struct RenderPassViewData
    {
        Camera camera;
        rhi::ViewConstants viewConstants;
        rhi::ScreenDimensions screenDimensions;
    };

    // Off-screen render target owned by a viewport (its final image), exposed as an NVRHI texture.
    struct RTVData
    {
        nvrhi::TextureHandle resource;
    };
}
