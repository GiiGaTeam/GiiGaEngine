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
            descriptor_.getGPUHandle();
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
}