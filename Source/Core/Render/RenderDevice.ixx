export module RenderDevice;

import <cassert>;
import <d3d12.h>;
import <memory>;
import <unordered_map>;
import <any>;
import <iostream>;

export import IRenderDevice;
import DirectXUtils;
import RenderSystemSettings;
import DescriptorHeap;
import BufferView;
import unique_any;

namespace GiiGa
{
    export class RenderDevice : public IRenderDevice
    {
        friend class RenderSystem;
        //todo: temp
        friend class RenderContext;
        friend class EditorSwapChainPass;

    private:
        RenderDevice():
            IRenderDevice(RenderSystemSettings::NUM_BACK_BUFFERS),
            device_(std::shared_ptr<ID3D12Device>{CreateDevice(), DXDeleter{}}),
            m_CPUDescriptorHeaps
            {
                {
                    *this, RenderSystemSettings::CPUDescriptorHeapAllocationSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                    D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                },
                {
                    *this, RenderSystemSettings::CPUDescriptorHeapAllocationSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                    D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                },
                {
                    *this, RenderSystemSettings::CPUDescriptorHeapAllocationSize[2], D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                    D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                },
                {
                    *this, RenderSystemSettings::CPUDescriptorHeapAllocationSize[3], D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                    D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                }
            },
            m_GPUDescriptorHeaps
            {
                {
                    *this, RenderSystemSettings::GPUDescriptorHeapSize[0], RenderSystemSettings::GPUDescriptorHeapDynamicSize[0],
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                },
                {
                    *this, RenderSystemSettings::GPUDescriptorHeapSize[1], RenderSystemSettings::GPUDescriptorHeapDynamicSize[1],
                    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                }
            }
        {
        }

        GPUDescriptorHeap& GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
        {
            return m_GPUDescriptorHeaps[type];
        }

    public:
        ~RenderDevice() override
        {
            for (auto&& vec : delete_queue)
            {
                vec.clear();
            }
        }

        std::shared_ptr<ID3D12Device> GetDevice()
        {
            return device_;
        }

        ///////////////////////// Create INTERFACE /////////////////////////////////////////////////

        std::shared_ptr<ID3D12CommandQueue> CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& desc)
        {
            if (!device_) return nullptr;

            ID3D12CommandQueue* d3d12CommandQueue;
            device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
            return std::shared_ptr<ID3D12CommandQueue>(d3d12CommandQueue, DXDeleter());
        }

        std::shared_ptr<ID3D12Fence> CreateFence(uint64_t fence_value, D3D12_FENCE_FLAGS flags)
        {
            if (!device_) return nullptr;

            ID3D12Fence* fence;
            device_->CreateFence(fence_value, flags, IID_PPV_ARGS(&fence));
            return std::shared_ptr<ID3D12Fence>(fence, DXDeleter());
        }

        std::shared_ptr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
        {
            if (!device_) return nullptr;
            ID3D12CommandAllocator* cmdAlloc;
            device_->CreateCommandAllocator(type,IID_PPV_ARGS(&cmdAlloc));
            return std::shared_ptr<ID3D12CommandAllocator>(cmdAlloc, DXDeleter());
        }

        // created on only one command allocator, strange 
        std::shared_ptr<ID3D12GraphicsCommandList> CreateGraphicsCommandList(D3D12_COMMAND_LIST_TYPE type,
                                                                             const std::shared_ptr<ID3D12CommandAllocator> command_allocator)
        {
            if (!device_) return nullptr;
            ID3D12GraphicsCommandList* cmdList;
            device_->CreateCommandList(0, type, command_allocator.get(), nullptr,IID_PPV_ARGS(&cmdList));
            return std::shared_ptr<ID3D12GraphicsCommandList>(cmdList, DXDeleter());
        }

        ///////////////////////// HEAPS INTERFACE /////////////////////////////////////////////////

        std::shared_ptr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc) override
        {
            if (!device_) return nullptr;
            ID3D12DescriptorHeap* d3d12DescriptorHeap;
            ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&d3d12DescriptorHeap)));
            return std::shared_ptr<ID3D12DescriptorHeap>(d3d12DescriptorHeap, DXDeleter());
        }

        UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) override
        {
            return device_->GetDescriptorHandleIncrementSize(type);
        }

        DynamicSuballocationsManager CreateDynamicSubAllocationManager(D3D12_DESCRIPTOR_HEAP_TYPE heap_type)
        {
            static uint32_t allocated_descriptors[2] = {0, 0};

            uint32_t alloc_size = RenderSystemSettings::GPUDescriptorHeapDynamicSize[heap_type] / RenderSystemSettings::
                NUM_FRAMES_IN_FLIGHT;

            assert(allocated_descriptors[heap_type] + alloc_size <= RenderSystemSettings::GPUDescriptorHeapDynamicSize[heap_type]);

            allocated_descriptors[heap_type] += alloc_size;

            return DynamicSuballocationsManager(m_GPUDescriptorHeaps[heap_type], alloc_size, "MB TODO");
        }

        ///////////////////////// BUFFERS INTERFACE /////////////////////////////////////////////////

        std::shared_ptr<ID3D12Resource> CreateCommittedResource(D3D12_HEAP_PROPERTIES pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, D3D12_RESOURCE_DESC pDesc,
                                                                D3D12_RESOURCE_STATES InitialResourceState, D3D12_CLEAR_VALUE* clearValue = nullptr) override
        {
            if (!device_) return nullptr;
            ID3D12Resource* d3d12Resource;
            device_->CreateCommittedResource(&pHeapProperties, HeapFlags, &pDesc, InitialResourceState, clearValue,
                                             IID_PPV_ARGS(&d3d12Resource));
            return std::shared_ptr<ID3D12Resource>(d3d12Resource, DXDelayedDeleter(*this));
        }

        std::shared_ptr<BufferView<Constant>> CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
        {
            DescriptorHeapAllocation cpuAlloc = m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
            DescriptorHeapAllocation gpuAlloc = m_GPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
            device_->CreateConstantBufferView(&desc, cpuAlloc.GetCpuHandle());

            device_->CopyDescriptorsSimple(1, gpuAlloc.GetCpuHandle(), cpuAlloc.GetCpuHandle(),
                                           D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            DesciptorHandles handles(std::move(cpuAlloc), std::move(gpuAlloc));

            return std::make_shared<BufferView<Constant>>(std::move(handles));
        }

        std::shared_ptr<BufferView<ShaderResource>> CreateShaderResourceBufferView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                                   const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
        {
            DescriptorHeapAllocation cpuAlloc = m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
            DescriptorHeapAllocation gpuAlloc = m_GPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
            device_->CreateShaderResourceView(buffer.get(), &desc, cpuAlloc.GetCpuHandle());

            device_->CopyDescriptorsSimple(1, gpuAlloc.GetCpuHandle(), cpuAlloc.GetCpuHandle(),
                                           D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            DesciptorHandles handles(std::move(cpuAlloc), std::move(gpuAlloc));

            return std::make_shared<BufferView<ShaderResource>>(std::move(handles));
        }

        std::shared_ptr<BufferView<UnorderedAccess>> CreateUnorderedAccessView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                               const std::shared_ptr<ID3D12Resource>& counterBuffer,
                                                                               const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
        {
            DescriptorHeapAllocation cpuAlloc = m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
            DescriptorHeapAllocation gpuAlloc = m_GPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(1);
            device_->CreateUnorderedAccessView(buffer.get(), counterBuffer.get(), &desc, cpuAlloc.GetCpuHandle());

            device_->CopyDescriptorsSimple(1, gpuAlloc.GetCpuHandle(), cpuAlloc.GetCpuHandle(),
                                           D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            DesciptorHandles handles(std::move(cpuAlloc), std::move(gpuAlloc));

            return std::make_shared<BufferView<UnorderedAccess>>(std::move(handles));
        }

        std::shared_ptr<BufferView<Constant>> CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc,
                                                                       DescriptorHeapAllocation&& gpuAlloc)
        {
            device_->CreateConstantBufferView(&desc, gpuAlloc.GetCpuHandle());

            DesciptorHandles handles({}, std::move(gpuAlloc));

            return std::make_shared<BufferView<Constant>>(std::move(handles));
        }

        std::shared_ptr<BufferView<ShaderResource>> CreateShaderResourceBufferView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                                   const D3D12_SHADER_RESOURCE_VIEW_DESC& desc,
                                                                                   DescriptorHeapAllocation&& gpuAlloc)
        {
            device_->CreateShaderResourceView(buffer.get(), &desc, gpuAlloc.GetCpuHandle());

            DesciptorHandles handles({}, std::move(gpuAlloc));

            return std::make_shared<BufferView<ShaderResource>>(std::move(handles));
        }

        std::shared_ptr<BufferView<UnorderedAccess>> CreateUnorderedAccessView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                               const std::shared_ptr<ID3D12Resource>& counterBuffer,
                                                                               const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc,
                                                                               DescriptorHeapAllocation&& gpuAlloc)
        {
            device_->CreateUnorderedAccessView(buffer.get(), counterBuffer.get(), &desc, gpuAlloc.GetCpuHandle());

            DesciptorHandles handles({}, std::move(gpuAlloc));

            return std::make_shared<BufferView<UnorderedAccess>>(std::move(handles));
        }

        std::shared_ptr<BufferView<RenderTarget>> CreateRenderTargetView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                         const D3D12_RENDER_TARGET_VIEW_DESC* desc)
        {
            DescriptorHeapAllocation cpuAlloc = m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Allocate(1);
            device_->CreateRenderTargetView(buffer.get(), desc, cpuAlloc.GetCpuHandle());

            return std::make_shared<BufferView<RenderTarget>>(std::move(cpuAlloc));
        }

        std::shared_ptr<BufferView<DepthStencil>> CreateDepthStencilView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                         const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
        {
            DescriptorHeapAllocation cpuAlloc = m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Allocate(1);
            device_->CreateDepthStencilView(buffer.get(), &desc, cpuAlloc.GetCpuHandle());

            return std::make_shared<BufferView<DepthStencil>>(std::move(cpuAlloc));
        }

        std::shared_ptr<BufferView<Index>> CreateIndexBufferView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                 D3D12_INDEX_BUFFER_VIEW& desc)
        {
            desc.BufferLocation = buffer->GetGPUVirtualAddress();

            return std::make_shared<BufferView<Index>>(std::move(desc));
        }

        std::shared_ptr<BufferView<Vertex>> CreateVetexBufferView(const std::shared_ptr<ID3D12Resource>& buffer,
                                                                  D3D12_VERTEX_BUFFER_VIEW& desc)
        {
            desc.BufferLocation = buffer->GetGPUVirtualAddress();

            return std::make_shared<BufferView<Vertex>>(std::move(desc));
        }

        std::shared_ptr<ID3D12Device> GetDxDevice() {
            return device_;
        }

    private:
        std::shared_ptr<ID3D12Device> device_;

        CPUDescriptorHeap m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        GPUDescriptorHeap m_GPUDescriptorHeaps[2];

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
    };
}
