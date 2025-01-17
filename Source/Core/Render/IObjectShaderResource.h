#pragma once

#include<vector>

namespace GiiGa
{
    struct IObjectShaderResource
    {
        virtual ~IObjectShaderResource() = default;
        virtual std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors()=0;
    };
}