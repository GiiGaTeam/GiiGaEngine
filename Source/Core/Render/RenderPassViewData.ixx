module;

#include<d3d12.h>
#include<directxtk12/SimpleMath.h>

export module RenderPassViewData;

import <memory>;

namespace GiiGa
{
    export struct RenderPassViewData
    {
        DirectX::SimpleMath::Matrix viewProjMatrix;
        D3D12_GPU_DESCRIPTOR_HANDLE camera_gpu_handle;
    };
}
