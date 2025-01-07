module;

#include<d3d12.h>
#include<directxtk12/SimpleMath.h>

export module RenderPassViewData;

import <memory>;

namespace GiiGa
{
    export struct alignas(256) RenderPassViewMatricies
    {
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projMatrix;
        DirectX::SimpleMath::Matrix invViewMatrix;
        DirectX::SimpleMath::Matrix invProjMatrix;
        DirectX::SimpleMath::Vector3 camPos;
    };
    
    export struct alignas(256) ScreenDimensions
    {
        DirectX::SimpleMath::Vector2 screenDimensions;
    };

    export struct RenderPassViewData
    {
        DirectX::SimpleMath::Matrix viewProjMat;
        D3D12_GPU_DESCRIPTOR_HANDLE viewDescriptor;
        ScreenDimensions screenDimensions;
        D3D12_GPU_DESCRIPTOR_HANDLE dimensionsDescriptor;
    };
}
