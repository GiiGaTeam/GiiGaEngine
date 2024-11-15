module;

#include<memory>
#include<d3d12.h>

export module IRenderDevice;

namespace GiiGa
{
    export struct IRenderDevice
    {
        virtual ~IRenderDevice() = default;
        virtual std::shared_ptr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc) =0;
        virtual UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) =0;
    };
}