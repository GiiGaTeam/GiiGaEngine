module;

#include <memory>
#include <d3d12.h>

export module BufferView;

export import ViewTypes;
import DescriptorHeap;
import IRenderDevice;

namespace GiiGa
{
    export template <typename ViewType>
    class BufferView
    {
        DesciptorHandles descriptor_;

    public:
        BufferView(DesciptorHandles&& descriptor_holder): descriptor_(std::move(descriptor_holder))
        {

        }

        D3D12_GPU_DESCRIPTOR_HANDLE getDescriptor()
        {
            return descriptor_.getGPUHandle();
        }
    };

    export template <>
    class BufferView<Index>
    {
        D3D12_INDEX_BUFFER_VIEW descriptor_;

    public:
        BufferView(D3D12_INDEX_BUFFER_VIEW desc): descriptor_(desc)
        {
        }

        D3D12_INDEX_BUFFER_VIEW getDescriptor()
        {
            return descriptor_;
        }
    };

    export template <>
    class BufferView<Vertex>
    {
        D3D12_VERTEX_BUFFER_VIEW descriptor_;

    public:
        BufferView(D3D12_VERTEX_BUFFER_VIEW desc): descriptor_(desc)
        {
        }

        D3D12_VERTEX_BUFFER_VIEW getDescriptor()
        {
            return descriptor_;
        }
    };

    //todo: generalize with consept or someth
    export template <>
    class BufferView<RenderTarget>
    {
        DescriptorHeapAllocation descriptor_;

    public:
        BufferView(DescriptorHeapAllocation&& descriptor_holder): descriptor_(std::move(descriptor_holder))
        {

        }

        D3D12_CPU_DESCRIPTOR_HANDLE getDescriptor()
        {
            return descriptor_.GetCpuHandle();
        }
    };

    export template <>
    class BufferView<DepthStencil>
    {
        DescriptorHeapAllocation descriptor_;

    public:
        BufferView(DescriptorHeapAllocation&& descriptor_holder): descriptor_(std::move(descriptor_holder))
        {

        }

        D3D12_CPU_DESCRIPTOR_HANDLE getDescriptor()
        {
            return descriptor_.GetCpuHandle();
        }
    };
}