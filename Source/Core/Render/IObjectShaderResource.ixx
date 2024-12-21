module;

#include<d3d12.h>

export module IObjectShaderResource;

import <vector>;

namespace GiiGa
{
    export struct IObjectShaderResource
    {
        virtual ~IObjectShaderResource() = default;
        virtual std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors()=0;
    };
}