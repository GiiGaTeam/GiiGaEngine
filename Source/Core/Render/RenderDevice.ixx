module;

#include <d3d12.h>
#include <memory>
#include <unordered_map>

export module RenderDevice;
import DirectXUtils;
import RenderSystemSettings;

namespace GiiGa
{
    export class RenderDevice
    {
    public:
        void Create()
        {
            device_ = std::shared_ptr<ID3D12Device>(CreateDevice(), DirectXDeleter());

        }

        std::shared_ptr<ID3D12CommandQueue> CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& desc) const
        {
            if (!device_) return nullptr;

            ID3D12CommandQueue* d3d12CommandQueue;
            device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
            return std::shared_ptr<ID3D12CommandQueue>(d3d12CommandQueue, DirectXDeleter());
        }

        std::shared_ptr<ID3D12Fence> CreateFence(uint64_t fence_value, D3D12_FENCE_FLAGS flags) const
        {
            if (!device_) return nullptr;

            ID3D12Fence* fence;
            device_->CreateFence(fence_value, flags, IID_PPV_ARGS(&fence));
            return std::shared_ptr<ID3D12Fence>(fence, DirectXDeleter());
        }

        std::shared_ptr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
        {
            if (!device_) return nullptr;
            ID3D12CommandAllocator* cmdAlloc;
            device_->CreateCommandAllocator(type,IID_PPV_ARGS(&cmdAlloc));
            return std::shared_ptr<ID3D12CommandAllocator>(cmdAlloc, DirectXDeleter());
        }

        // created on only one command allocator, strange 
        std::shared_ptr<ID3D12GraphicsCommandList> CreateGraphicsCommandList(D3D12_COMMAND_LIST_TYPE type,
            const std::shared_ptr<ID3D12CommandAllocator> command_allocator)
        {
            if (!device_) return nullptr;
            ID3D12GraphicsCommandList* cmdList;
            device_->CreateCommandList(0, type, command_allocator.get(), nullptr,IID_PPV_ARGS(&cmdList));
            return std::shared_ptr<ID3D12GraphicsCommandList>(cmdList, DirectXDeleter());
        }

    private:
        std::shared_ptr<ID3D12Device> device_;

        std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<ID3D12DescriptorHeap>> descriptor_heaps_;

        ID3D12Device* CreateDevice()
        {
            ID3D12Device* device;
            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

#ifndef NDEBUG
            ID3D12Debug* pdx12Debug = nullptr;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
                pdx12Debug->EnableDebugLayer();
#endif

            ThrowIfFailed(D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&device)));

#ifndef NDEBUG
            if (pdx12Debug != nullptr)
            {
                ID3D12InfoQueue* pInfoQueue = nullptr;
                device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                pInfoQueue->Release();
                pdx12Debug->Release();
            }
#endif

            return device;
        }

        // NOTE: for now we assume:
        //      number of RTVs = NUM_BACK_BUFFERS
        //      number of CBV_SRV_UAV = NUM_CBV_SRV_UAVs
        void CreateHeaps()
        {
            // create rtv heap
            {
                ID3D12DescriptorHeap* desc_heap;
                const UINT num_rtvs = RenderSystemSettings::NUM_BACK_BUFFERS;

                D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                desc.NumDescriptors = num_rtvs;
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                desc.NodeMask = 1;

                ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&desc_heap)));

                SIZE_T rtvDescriptorSize = device_->GetDescriptorHandleIncrementSize(
                    D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = desc_heap->
                    GetCPUDescriptorHandleForHeapStart();

                // todo give render context descriptor handles
                for (UINT i = 0; i < num_rtvs; i++)
                {
                    g_mainRenderTargetDescriptor[i] = rtvHandle;
                    rtvHandle.ptr += rtvDescriptorSize;
                }
            }

            {
                D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                desc.NumDescriptors = 1;
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                if (g_pd3dDevice->CreateDescriptorHeap(
                        &desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
                    return false;
            }
        }
    };
}