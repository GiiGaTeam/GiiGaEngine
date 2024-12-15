module;

#include<memory>
#include<list>
#include<vector>
#include<d3d12.h>
#include<iostream>



export module IRenderDevice;

export import unique_any;

namespace GiiGa
{
    export struct IRenderDevice
    {
        IRenderDevice(int count):
            delete_queue(count)
        {
        }

        virtual ~IRenderDevice()
        {
            for (auto&& vec : delete_queue)
            {
                vec.clear();
            }
        }

        virtual std::shared_ptr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc) =0;
        virtual UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) =0;
        virtual std::shared_ptr<ID3D12Resource> CreateCommittedResource(D3D12_HEAP_PROPERTIES pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, D3D12_RESOURCE_DESC pDesc,
                                                                        D3D12_RESOURCE_STATES InitialResourceState, D3D12_CLEAR_VALUE* clearValue = nullptr) =0;

        void EnqueueToDelete(unique_any to_destroy)
        {
            delete_queue.back().push_back(std::move(to_destroy));
        }

        void DeleteStaleObjects()
        {
            std::vector<unique_any> front = std::move(delete_queue.front());
            delete_queue.pop_front();

            // todo: may be just clear is enough
            for (auto&& el : front)
            {
                el.reset();
            }
            front.clear();

            delete_queue.push_back(std::move(front));
        }

    protected:
        std::list<std::vector<unique_any>> delete_queue;
    };
}
