#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>

#include<CameraComponent.h>

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

    struct RenderPassViewData
    {
        Camera camera;
        D3D12_GPU_DESCRIPTOR_HANDLE viewDescriptor;
        ScreenDimensions screenDimensions;
        D3D12_GPU_DESCRIPTOR_HANDLE dimensionsDescriptor;
    };
}
