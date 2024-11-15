module;

#include <d3d12.h>

export module ViewTypes;

import DescriptorHeap;

namespace GiiGa
{
    export class Constant
    {
        using DescriptorType = D3D12_GPU_DESCRIPTOR_HANDLE;
    };

    export class ShaderResource
    {
        using DescriptorType = D3D12_GPU_DESCRIPTOR_HANDLE;
    };

    export class UnorderedAccess
    {
        using DescriptorType = D3D12_GPU_DESCRIPTOR_HANDLE;
    };

    export class Vertex
    {
        using DescriptorType = D3D12_VERTEX_BUFFER_VIEW;
    };

    export class Index
    {
        using DescriptorType = D3D12_VERTEX_BUFFER_VIEW;
    };
}