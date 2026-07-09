#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>

#include<nvrhi/nvrhi.h>

#include<CameraComponent.h>
#include<RenderTypes.h>

namespace GiiGa
{
    struct alignas(256) RenderPassViewMatricies
    {
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projMatrix;
        DirectX::SimpleMath::Matrix invViewMatrix;
        DirectX::SimpleMath::Matrix invProjMatrix;
        DirectX::SimpleMath::Vector3 camPos;
    };

    struct alignas(256) ScreenDimensions
    {
        DirectX::SimpleMath::Vector2 screenDimensions;
    };

    // Per-camera view info: the camera plus the NVRHI constant buffers holding the
    // view matrices (b0) and screen dimensions, filled each frame by the viewport.
    struct RenderPassViewData
    {
        Camera camera;
        nvrhi::IBuffer* viewCB = nullptr;         // register b0
        ScreenDimensions screenDimensions;
        nvrhi::IBuffer* dimensionsCB = nullptr;
    };

    // The colour result of a viewport's render graph: a texture (+ framebuffer to render
    // into). Its SRV (for ImGui::Image display) is obtained on demand from the texture.
    struct RTVData
    {
        nvrhi::TextureHandle resource_;
        nvrhi::FramebufferHandle framebuffer_;
    };
}
