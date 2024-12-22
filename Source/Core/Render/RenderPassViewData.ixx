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
    };
}
