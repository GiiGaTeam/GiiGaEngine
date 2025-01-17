#pragma once


#include<DescriptorHeap.h>

namespace GiiGa
{
    struct Constant
    {
        using ViewHolder = DesciptorHandles;
        using ViewDesc = D3D12_CONSTANT_BUFFER_VIEW_DESC;
    };

    struct ShaderResource
    {
        using ViewHolder = DesciptorHandles;
        using ViewDesc = D3D12_SHADER_RESOURCE_VIEW_DESC;
    };

    struct UnorderedAccess
    {
        using ViewHolder = DesciptorHandles;
        using ViewDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC;
    };

    struct RenderTarget
    {
        using ViewHolder = DescriptorHeapAllocation;
        using ViewDesc = D3D12_RENDER_TARGET_VIEW_DESC;
    };

    struct DepthStencil
    {
        using ViewHolder = DescriptorHeapAllocation;
        using ViewDesc = D3D12_DEPTH_STENCIL_VIEW_DESC;
    };

    struct Vertex
    {
        using ViewHolder = D3D12_VERTEX_BUFFER_VIEW;
        using ViewDesc = D3D12_VERTEX_BUFFER_VIEW;
    };

    struct Index
    {
        using ViewHolder = D3D12_INDEX_BUFFER_VIEW;
        using ViewDesc = D3D12_INDEX_BUFFER_VIEW;
    };
}