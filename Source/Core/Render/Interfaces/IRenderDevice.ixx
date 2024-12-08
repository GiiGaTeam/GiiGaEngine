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
        virtual std::shared_ptr<ID3D12Resource> CreateCommittedResource(D3D12_HEAP_PROPERTIES pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, D3D12_RESOURCE_DESC pDesc,
                                                                        D3D12_RESOURCE_STATES InitialResourceState, D3D12_CLEAR_VALUE* clearValue = nullptr) =0;
    };
}
