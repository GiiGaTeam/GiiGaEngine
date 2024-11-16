module;

#include <algorithm>
#define NOMINMAX
#include <d3d12.h>
#include <cstdint>
#include <limits>
#include <cassert>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

export module DescriptorHeap;

import VariableSizeAllocationsManager;
import IRenderDevice;

// TODO:
// free stale enqueueing
// DILIGENT_DEVELOPMENT
// add LOG_ERROR, LOG_INFO_MESSAGE

namespace GiiGa
{
    export class DescriptorHeapAllocation;
    class DescriptorHeapAllocationManager;

    class IDescriptorAllocator
    {
    public:
        virtual ~IDescriptorAllocator() = default;
        // Allocate Count descriptors
        virtual DescriptorHeapAllocation Allocate(uint32_t Count) = 0;
        virtual void Free(DescriptorHeapAllocation&& Allocation, uint64_t CmdQueueMask) = 0;
        virtual uint32_t GetDescriptorSize() const = 0;
    };


    // The class represents descriptor heap allocation (continuous descriptor range in a descriptor heap)
    //
    //                  m_FirstCpuHandle
    //                   |
    //  | ~  ~  ~  ~  ~  X  X  X  X  X  X  X  ~  ~  ~  ~  ~  ~ |  D3D12 Descriptor Heap
    //                   |
    //                  m_FirstGpuHandle
    //
    export class DescriptorHeapAllocation
    {
    public:
        // Creates null allocation
        DescriptorHeapAllocation() noexcept :
            // clang-format off
            m_pDescriptorHeap{nullptr},
            m_NumHandles{1}, // One null descriptor handle
            m_DescriptorSize{0}
        // clang-format on
        {
            m_FirstCpuHandle.ptr = 0;
            m_FirstGpuHandle.ptr = 0;
        }

        // Initializes non-null allocation
        DescriptorHeapAllocation(IDescriptorAllocator& Allocator,
            std::shared_ptr<ID3D12DescriptorHeap> pHeap,
            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
            uint32_t NHandles,
            uint16_t AllocationManagerId) noexcept :
            // clang-format off
            m_FirstCpuHandle{CpuHandle},
            m_FirstGpuHandle{GpuHandle},
            m_pAllocator{&Allocator},
            m_pDescriptorHeap{pHeap},
            m_NumHandles{NHandles},
            m_AllocationManagerId{AllocationManagerId}
        // clang-format on
        {
            assert(m_pAllocator != nullptr && m_pDescriptorHeap != nullptr);
            uint32_t DescriptorSize = m_pAllocator->GetDescriptorSize();
            assert(DescriptorSize < std::numeric_limits<uint16_t>::max());
            //"DescriptorSize exceeds allowed limit"
            m_DescriptorSize = static_cast<uint16_t>(DescriptorSize);
        }

        // Move constructor (copy is not allowed)
        DescriptorHeapAllocation(DescriptorHeapAllocation&& Allocation) noexcept :
            // clang-format off
            m_FirstCpuHandle{std::move(Allocation.m_FirstCpuHandle)},
            m_FirstGpuHandle{std::move(Allocation.m_FirstGpuHandle)},
            m_pAllocator{std::move(Allocation.m_pAllocator)},
            m_pDescriptorHeap{std::move(Allocation.m_pDescriptorHeap)},
            m_NumHandles{std::move(Allocation.m_NumHandles)},
            m_AllocationManagerId{std::move(Allocation.m_AllocationManagerId)},
            m_DescriptorSize{std::move(Allocation.m_DescriptorSize)}
        // clang-format on
        {
            Allocation.Reset();
        }

        // Move assignment (assignment is not allowed)
        DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& Allocation) noexcept
        {
            m_FirstCpuHandle = std::move(Allocation.m_FirstCpuHandle);
            m_FirstGpuHandle = std::move(Allocation.m_FirstGpuHandle);
            m_NumHandles = std::move(Allocation.m_NumHandles);
            m_pAllocator = std::move(Allocation.m_pAllocator);
            m_AllocationManagerId = std::move(Allocation.m_AllocationManagerId);
            m_pDescriptorHeap = std::move(Allocation.m_pDescriptorHeap);
            m_DescriptorSize = std::move(Allocation.m_DescriptorSize);

            Allocation.Reset();

            return *this;
        }

        void Reset()
        {
            m_FirstCpuHandle.ptr = 0;
            m_FirstGpuHandle.ptr = 0;
            m_pAllocator = nullptr;
            m_pDescriptorHeap = nullptr;
            m_NumHandles = 0;
            m_AllocationManagerId = InvalidAllocationMgrId;
            m_DescriptorSize = 0;
        }

        // clang-format off
        DescriptorHeapAllocation(const DescriptorHeapAllocation&) = delete;
        DescriptorHeapAllocation& operator=(const DescriptorHeapAllocation&) = delete;
        // clang-format on


        // Destructor automatically releases this allocation through the allocator
        ~DescriptorHeapAllocation()
        {
            if (!IsNull() && m_pAllocator)
                m_pAllocator->Free(std::move(*this), ~uint64_t{0});
            // Allocation must have been disposed by the allocator
            assert(IsNull()); //"Non-null descriptor is being destroyed");
        }

        // Returns CPU descriptor handle at the specified offset
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t Offset = 0) const
        {
            assert(Offset >= 0 && Offset < m_NumHandles);

            D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_FirstCpuHandle;
            CPUHandle.ptr += SIZE_T{m_DescriptorSize} * SIZE_T{Offset};

            return CPUHandle;
        }

        // Returns GPU descriptor handle at the specified offset
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t Offset = 0) const
        {
            assert(Offset >= 0 && Offset < m_NumHandles);
            D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = m_FirstGpuHandle;
            GPUHandle.ptr += SIZE_T{m_DescriptorSize} * SIZE_T{Offset};

            return GPUHandle;
        }

        template <typename HandleType>
        HandleType GetHandle(uint32_t Offset = 0) const;

        template <>
        D3D12_CPU_DESCRIPTOR_HANDLE GetHandle<D3D12_CPU_DESCRIPTOR_HANDLE>(uint32_t Offset) const
        {
            return GetCpuHandle(Offset);
        }

        template <>
        D3D12_GPU_DESCRIPTOR_HANDLE GetHandle<D3D12_GPU_DESCRIPTOR_HANDLE>(uint32_t Offset) const
        {
            return GetGpuHandle(Offset);
        }


        // Returns pointer to D3D12 descriptor heap that contains this allocation
        std::shared_ptr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return m_pDescriptorHeap; }


        // clang-format off
        size_t GetNumHandles() const { return m_NumHandles; }
        bool IsNull() const { return m_FirstCpuHandle.ptr == 0; }
        bool IsShaderVisible() const { return m_FirstGpuHandle.ptr != 0; }
        size_t GetAllocationManagerId() const { return m_AllocationManagerId; }
        UINT GetDescriptorSize() const { return m_DescriptorSize; }
        // clang-format on

    private:
        // First CPU descriptor handle in this allocation
        D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuHandle = {0};

        // First GPU descriptor handle in this allocation
        D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuHandle = {0};

        // Keep strong reference to the parent heap to make sure it is alive while allocation is alive - TOO EXPENSIVE
        //RefCntAutoPtr<IDescriptorAllocator> m_pAllocator;

        // Pointer to the descriptor heap allocator that created this allocation
        IDescriptorAllocator* m_pAllocator = nullptr;

        // Pointer to the D3D12 descriptor heap that contains descriptors in this allocation
        std::shared_ptr<ID3D12DescriptorHeap> m_pDescriptorHeap = nullptr;

        // Number of descriptors in the allocation
        uint32_t m_NumHandles = 0;

        static constexpr uint16_t InvalidAllocationMgrId = 0xFFFF;

        // Allocation manager ID. One allocator may support several
        // allocation managers. This field is required to identify
        // the manager within the allocator that was used to create
        // this allocation
        uint16_t m_AllocationManagerId = InvalidAllocationMgrId;

        // Descriptor size
        uint16_t m_DescriptorSize = 0;
    };

    export class DesciptorHandles
    {
        DescriptorHeapAllocation cpu_alloc_;
        DescriptorHeapAllocation gpu_alloc_;

    public:
        DesciptorHandles(DescriptorHeapAllocation&& cpuAlloc, DescriptorHeapAllocation&& gpuAlloc):
            cpu_alloc_(std::move(cpuAlloc)), gpu_alloc_(std::move(gpuAlloc))
        {
        }

        D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle()
        {
            return gpu_alloc_.GetGpuHandle();
        }
    };

    // The class performs suballocations within one D3D12 descriptor heap.
    // It uses VariableSizeAllocationsManager to manage free space in the heap
    //
    // |  X  X  X  X  O  O  O  X  X  O  O  X  O  O  O  O  |  D3D12 descriptor heap
    //
    //  X - used descriptor
    //  O - available descriptor
    //
    class DescriptorHeapAllocationManager
    {
    public:
        // Creates a new D3D12 descriptor heap
        DescriptorHeapAllocationManager(
            IRenderDevice& DeviceD3D12Impl,
            IDescriptorAllocator& ParentAllocator,
            size_t ThisManagerId,
            const D3D12_DESCRIPTOR_HEAP_DESC& HeapDesc) :
            DescriptorHeapAllocationManager //
            {
                DeviceD3D12Impl,
                ParentAllocator,
                ThisManagerId,
                [&HeapDesc, &DeviceD3D12Impl]() -> std::shared_ptr<ID3D12DescriptorHeap> //
                {
                    return DeviceD3D12Impl.CreateDescriptorHeap(HeapDesc);
                }(),
                0, // First descriptor
                HeapDesc.NumDescriptors // Num descriptors
            }
        {
        }


        // Uses subrange of descriptors in the existing D3D12 descriptor heap
        // that starts at offset FirstDescriptor and uses NumDescriptors descriptors
        DescriptorHeapAllocationManager(
            IRenderDevice& DeviceD3D12Impl,
            IDescriptorAllocator& ParentAllocator,
            size_t ThisManagerId,
            std::shared_ptr<ID3D12DescriptorHeap> pd3d12DescriptorHeap,
            uint32_t FirstDescriptor,
            uint32_t NumDescriptors) :
            // clang-format off
            m_ParentAllocator{ParentAllocator},
            m_DeviceD3D12Impl{DeviceD3D12Impl},
            m_ThisManagerId{ThisManagerId},
            m_HeapDesc{pd3d12DescriptorHeap->GetDesc()},
            m_DescriptorSize{DeviceD3D12Impl.GetDescriptorHandleIncrementSize(m_HeapDesc.Type)},
            m_NumDescriptorsInAllocation{NumDescriptors},
            m_FreeBlockManager{NumDescriptors},
            m_pd3d12DescriptorHeap{pd3d12DescriptorHeap}
        // clang-format on
        {
            m_FirstCPUHandle = pd3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            m_FirstCPUHandle.ptr += SIZE_T{m_DescriptorSize} * SIZE_T{FirstDescriptor};

            if (m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
            {
                m_FirstGPUHandle = pd3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
                m_FirstGPUHandle.ptr += SIZE_T{m_DescriptorSize} * SIZE_T{FirstDescriptor};
            }

#ifdef DILIGENT_DEVELOPMENT
    {
        auto InvalidHeapDesc           = m_HeapDesc;
        InvalidHeapDesc.NumDescriptors = InvalidDescriptorsCount;
        InvalidHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        m_pd3d12InvalidDescriptorHeap  = CreateInvalidDescriptorHeap(DeviceD3D12Impl.GetD3D12Device(), InvalidHeapDesc);
    }
#endif
        }


        // = default causes compiler error when instantiating std::vector::emplace_back() in Visual Studio 2015 (Version 14.0.23107.0 D14REL)
        DescriptorHeapAllocationManager(DescriptorHeapAllocationManager&& rhs) noexcept :
            // clang-format off
            m_ParentAllocator{rhs.m_ParentAllocator},
            m_DeviceD3D12Impl{rhs.m_DeviceD3D12Impl},
            m_ThisManagerId{rhs.m_ThisManagerId},
            m_HeapDesc{rhs.m_HeapDesc},
            m_DescriptorSize{rhs.m_DescriptorSize},
            m_NumDescriptorsInAllocation{rhs.m_NumDescriptorsInAllocation},
            // Mutex is not movable
            //m_FreeBlockManagerMutex     (std::move(rhs.m_FreeBlockManagerMutex))
            m_FreeBlockManager{std::move(rhs.m_FreeBlockManager)},
            m_pd3d12DescriptorHeap{std::move(rhs.m_pd3d12DescriptorHeap)},
            m_FirstCPUHandle{rhs.m_FirstCPUHandle},
            m_FirstGPUHandle{rhs.m_FirstGPUHandle},
            m_MaxAllocatedSize{rhs.m_MaxAllocatedSize}
        // clang-format on
        {
            rhs.m_NumDescriptorsInAllocation = 0; // Must be set to zero so that debug check in dtor passes
            rhs.m_ThisManagerId = static_cast<size_t>(-1);
            rhs.m_FirstCPUHandle.ptr = 0;
            rhs.m_FirstGPUHandle.ptr = 0;
            rhs.m_MaxAllocatedSize = 0;
#ifdef DILIGENT_DEVELOPMENT
        m_AllocationsCounter.store(rhs.m_AllocationsCounter.load());
        rhs.m_AllocationsCounter      = 0;
        m_pd3d12InvalidDescriptorHeap = std::move(rhs.m_pd3d12InvalidDescriptorHeap);
#endif
        }

        // clang-format off
        // No copies or move-assignments
        DescriptorHeapAllocationManager& operator =(DescriptorHeapAllocationManager&&) = delete;
        DescriptorHeapAllocationManager(const DescriptorHeapAllocationManager&) = delete;
        DescriptorHeapAllocationManager& operator =(const DescriptorHeapAllocationManager&) = delete;
        // clang-format on
        
        // Allocates Count descriptors
        DescriptorHeapAllocation Allocate(uint32_t Count)

        {
            assert(Count > 0);

            std::lock_guard<std::mutex> LockGuard(m_FreeBlockManagerMutex);
            // Methods of VariableSizeAllocationsManager class are not thread safe!

            // Use variable-size GPU allocations manager to allocate the requested number of descriptors
            auto Allocation = m_FreeBlockManager.Allocate(Count, 1);
            if (!Allocation.IsValid())
                return DescriptorHeapAllocation{};

            assert(Allocation.Size == Count);

            // Compute the first CPU and GPU descriptor handles in the allocation by
            // offsetting the first CPU and GPU descriptor handle in the range
            auto CPUHandle = m_FirstCPUHandle;
            CPUHandle.ptr += Allocation.UnalignedOffset * m_DescriptorSize;

            auto GPUHandle = m_FirstGPUHandle; // Will be null if the heap is not GPU-visible
            if (m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                GPUHandle.ptr += Allocation.UnalignedOffset * m_DescriptorSize;

            m_MaxAllocatedSize = std::max(m_MaxAllocatedSize, m_FreeBlockManager.GetUsedSize());

#ifdef DILIGENT_DEVELOPMENT
    ++m_AllocationsCounter;
    // Copy invalid descriptors. If the descriptors are accessed, this will cause device removal.
    {
        auto* const pd3d12Device      = m_DeviceD3D12Impl.GetD3D12Device();
        const auto  InvalidCPUHandles = m_pd3d12InvalidDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (uint32_t FisrtDescr = 0; FisrtDescr < Count; FisrtDescr += InvalidDescriptorsCount)
        {
            auto NumDescrsToCopy = std::min(Count - FisrtDescr, InvalidDescriptorsCount);
            auto DstCPUHandle    = CPUHandle;
            DstCPUHandle.ptr += SIZE_T{FisrtDescr} * SIZE_T{m_DescriptorSize};
            pd3d12Device->CopyDescriptorsSimple(NumDescrsToCopy, DstCPUHandle, InvalidCPUHandles, m_HeapDesc.Type);
        }
    }
#endif

            assert(m_ThisManagerId < std::numeric_limits<uint16_t>::max()); //"ManagerID exceeds 16-bit range"

            return DescriptorHeapAllocation{m_ParentAllocator, m_pd3d12DescriptorHeap, CPUHandle, GPUHandle, Count,
                                            static_cast<uint16_t>(m_ThisManagerId)};
        }

        void FreeAllocation(DescriptorHeapAllocation&& Allocation)
        {
            assert(Allocation.GetAllocationManagerId() == m_ThisManagerId); //, "Invalid descriptor heap manager Id"

            if (Allocation.IsNull())
                return;

            std::lock_guard<std::mutex> LockGuard(m_FreeBlockManagerMutex);
            auto DescriptorOffset = (Allocation.GetCpuHandle().ptr - m_FirstCPUHandle.ptr) / m_DescriptorSize;
            // Methods of VariableSizeAllocationsManager class are not thread safe!
            m_FreeBlockManager.Free(DescriptorOffset, Allocation.GetNumHandles());

            // Clear the allocation
            Allocation.Reset();
#ifdef DILIGENT_DEVELOPMENT
            --m_AllocationsCounter;
#endif
        }

        // clang-format off
        size_t GetNumAvailableDescriptors() const { return m_FreeBlockManager.GetFreeSize(); }
        uint32_t GetMaxDescriptors() const { return m_NumDescriptorsInAllocation; }
        size_t GetMaxAllocatedSize() const { return m_MaxAllocatedSize; }
        // clang-format on

#ifdef DILIGENT_DEVELOPMENT
    Int32 DvpGetAllocationsCounter() const
    {
        return m_AllocationsCounter.load();
    }
#endif

    private:
        IDescriptorAllocator& m_ParentAllocator;
        IRenderDevice& m_DeviceD3D12Impl;

        // External ID assigned to this descriptor allocations manager
        size_t m_ThisManagerId = static_cast<size_t>(-1);

        // Heap description
        const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;

        const UINT m_DescriptorSize = 0;

        // Number of descriptors in the allocation.
        // If this manager was initialized as a subrange in the existing heap,
        // this value may be different from m_HeapDesc.NumDescriptors
        uint32_t m_NumDescriptorsInAllocation = 0;

        // Allocations manager used to handle descriptor allocations within the heap
        std::mutex m_FreeBlockManagerMutex;
        VariableSizeAllocationsManager m_FreeBlockManager;

        // Strong reference to D3D12 descriptor heap object
        std::shared_ptr<ID3D12DescriptorHeap> m_pd3d12DescriptorHeap;

        // First CPU descriptor handle in the available descriptor range
        D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCPUHandle = {0};

        // First GPU descriptor handle in the available descriptor range
        D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGPUHandle = {0};

        size_t m_MaxAllocatedSize = 0;

#ifdef DILIGENT_DEVELOPMENT
    std::atomic<Int32> m_AllocationsCounter{0};
    // This descriptor heap is only used to copy invalid descriptors to
    // a new allocated region. Using these descriptors will result in device
    // removal. Note that using null descriptors is perfectly valid in D3D12
    // and does not produce any errors.
    static constexpr uint32_t       InvalidDescriptorsCount = 128;
    CComPtr<ID3D12DescriptorHeap> m_pd3d12InvalidDescriptorHeap;
#endif

        // Note: when adding new members, do not forget to update move ctor
    };

    // CPU descriptor heap is intended to provide storage for resource view descriptor handles.
    // It contains a pool of DescriptorHeapAllocationManager object instances, where every instance manages
    // its own CPU-only D3D12 descriptor heap:
    //
    //           m_HeapPool[0]                m_HeapPool[1]                 m_HeapPool[2]
    //   |  X  X  X  X  X  X  X  X |, |  X  X  X  O  O  X  X  O  |, |  X  O  O  O  O  O  O  O  |
    //
    //    X - used descriptor                m_AvailableHeaps = {1,2}
    //    O - available descriptor
    //
    // Allocation routine goes through the list of managers that have available descriptors and tries to process
    // the request using every manager. If there are no available managers or no manager was able to handle the request,
    // the function creates a new descriptor heap manager and lets it handle the request
    //
    // Render device contains four CPUDescriptorHeap object instances (one for each D3D12 heap type). The heaps are accessed
    // when a texture or a buffer view is created.
    //
    export class CPUDescriptorHeap final : public IDescriptorAllocator
    {
    public:
        // Initializes the heap
        CPUDescriptorHeap(
            IRenderDevice& DeviceD3D12Impl,
            uint32_t NumDescriptorsInHeap,
            D3D12_DESCRIPTOR_HEAP_TYPE Type,
            D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
            :
            // clang-format off
            m_DeviceD3D12Impl{DeviceD3D12Impl},
            m_HeapPool(),
            m_AvailableHeaps(),
            m_HeapDesc
            {
                Type,
                NumDescriptorsInHeap,
                Flags,
                1 // NodeMask
            },
            m_DescriptorSize{DeviceD3D12Impl.GetDescriptorHandleIncrementSize(Type)}
        // clang-format on
        {
            // Create one pool
            m_HeapPool.emplace_back(m_DeviceD3D12Impl, *this, 0, m_HeapDesc);
            m_AvailableHeaps.insert(0);
        }

        // clang-format off
        CPUDescriptorHeap(const CPUDescriptorHeap&) = delete;
        CPUDescriptorHeap(CPUDescriptorHeap&&) = delete;
        CPUDescriptorHeap& operator =(const CPUDescriptorHeap&) = delete;
        CPUDescriptorHeap& operator =(CPUDescriptorHeap&&) = delete;
        // clang-format on

        ~CPUDescriptorHeap() override
        {
            assert(m_CurrentSize == 0); //, "Not all allocations released"

            assert(m_AvailableHeaps.size() == m_HeapPool.size()); //, "Not all descriptor heap pools are released"
            uint32_t TotalDescriptors = 0;
            for (auto& Heap : m_HeapPool)
            {
                assert(Heap.GetNumAvailableDescriptors() == Heap.GetMaxDescriptors());
                // , "Not all descriptors in the descriptor pool are released"
                TotalDescriptors += Heap.GetMaxDescriptors();
            }

            //LOG_INFO_MESSAGE(std::setw(38), std::left, GetD3D12DescriptorHeapTypeLiteralName(m_HeapDesc.Type),
            //    " CPU heap allocated pool count: ", m_HeapPool.size(),
            //    ". Max descriptors: ", m_MaxSize, '/', TotalDescriptors,
            //    " (", std::fixed, std::setprecision(2), m_MaxSize * 100.0 / std::max(TotalDescriptors, 1u), "%).");
        }

        virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final
        {
            std::lock_guard<std::mutex> LockGuard(m_HeapPoolMutex);
            // Note that every DescriptorHeapAllocationManager object instance is itself
            // thread-safe. Nested mutexes cannot cause a deadlock

            DescriptorHeapAllocation Allocation;
            // Go through all descriptor heap managers that have free descriptors
            auto AvailableHeapIt = m_AvailableHeaps.begin();
            while (AvailableHeapIt != m_AvailableHeaps.end())
            {
                auto NextIt = AvailableHeapIt;
                ++NextIt;
                // Try to allocate descriptor using the current descriptor heap manager
                Allocation = m_HeapPool[*AvailableHeapIt].Allocate(Count);
                // Remove the manager from the pool if it has no more available descriptors
                if (m_HeapPool[*AvailableHeapIt].GetNumAvailableDescriptors() == 0)
                    m_AvailableHeaps.erase(*AvailableHeapIt);

                // Terminate the loop if descriptor was successfully allocated, otherwise
                // go to the next manager
                if (!Allocation.IsNull())
                    break;
                AvailableHeapIt = NextIt;
            }

            // If there were no available descriptor heap managers or no manager was able
            // to suffice the allocation request, create a new manager
            if (Allocation.IsNull())
            {
                // Make sure the heap is large enough to accommodate the requested number of descriptors
                if (Count > m_HeapDesc.NumDescriptors)
                {
                    //LOG_INFO_MESSAGE("Number of requested CPU descriptors handles (", Count, ") exceeds the descriptor heap size (",
                    //    m_HeapDesc.NumDescriptors, "). Increasing the number of descriptors in the heap");
                }
                m_HeapDesc.NumDescriptors = std::max(m_HeapDesc.NumDescriptors, static_cast<UINT>(Count));
                // Create a new descriptor heap manager. Note that this constructor creates a new D3D12 descriptor
                // heap and references the entire heap. Pool index is used as manager ID
                m_HeapPool.emplace_back(m_DeviceD3D12Impl, *this, m_HeapPool.size(), m_HeapDesc);
                auto NewHeapIt = m_AvailableHeaps.insert(m_HeapPool.size() - 1);
                assert(NewHeapIt.second);

                // Use the new manager to allocate descriptor handles
                Allocation = m_HeapPool[*NewHeapIt.first].Allocate(Count);
            }

            m_CurrentSize += static_cast<uint32_t>(Allocation.GetNumHandles());
            m_MaxSize = std::max(m_MaxSize, m_CurrentSize);

            return Allocation;
        }

        virtual void Free(DescriptorHeapAllocation&& Allocation, uint64_t CmdQueueMask) override final
        {
            struct StaleAllocation
            {
                DescriptorHeapAllocation Allocation;
                CPUDescriptorHeap* Heap;

                // clang-format off
                StaleAllocation(DescriptorHeapAllocation&& _Allocation, CPUDescriptorHeap& _Heap) noexcept :
                    Allocation{std::move(_Allocation)},
                    Heap{&_Heap}
                {
                }

                StaleAllocation(const StaleAllocation&) = delete;
                StaleAllocation& operator=(const StaleAllocation&) = delete;
                StaleAllocation& operator=(StaleAllocation&&) = delete;

                StaleAllocation(StaleAllocation&& rhs) noexcept :
                    Allocation{std::move(rhs.Allocation)},
                    Heap{rhs.Heap}
                {
                    rhs.Heap = nullptr;
                }

                // clang-format on

                ~StaleAllocation()
                {
                    if (Heap != nullptr)
                        Heap->FreeAllocation(std::move(Allocation));
                }
            };
            // todo review here should be enqueueing
            StaleAllocation{std::move(Allocation), *this};
            //m_DeviceD3D12Impl.SafeReleaseDeviceObject(StaleAllocation{std::move(Allocation), *this}, CmdQueueMask);
        }

        virtual uint32_t GetDescriptorSize() const override final { return m_DescriptorSize; }

#ifdef DILIGENT_DEVELOPMENT
    int32_t DvpGetTotalAllocationCount();
#endif

    private:
        void FreeAllocation(DescriptorHeapAllocation&& Allocation)
        {
            std::lock_guard<std::mutex> LockGuard(m_HeapPoolMutex);
            auto ManagerId = Allocation.GetAllocationManagerId();
            m_CurrentSize -= static_cast<uint32_t>(Allocation.GetNumHandles());
            m_HeapPool[ManagerId].FreeAllocation(std::move(Allocation));
            // Return the manager to the pool of available managers
            assert(m_HeapPool[ManagerId].GetNumAvailableDescriptors() > 0);
            m_AvailableHeaps.insert(ManagerId);
        }

        IRenderDevice& m_DeviceD3D12Impl;

        // Pool of descriptor heap managers
        std::mutex m_HeapPoolMutex;
        std::vector<DescriptorHeapAllocationManager> m_HeapPool;
        // Indices of available descriptor heap managers
        std::unordered_set<size_t> m_AvailableHeaps;

        D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
        const UINT m_DescriptorSize = 0;

        // Maximum heap size during the application lifetime - for statistic purposes
        uint32_t m_MaxSize = 0;
        uint32_t m_CurrentSize = 0;
    };

    // GPU descriptor heap provides storage for shader-visible descriptors
    // The heap contains single D3D12 descriptor heap that is split into two parts.
    // The first part stores static and mutable resource descriptor handles.
    // The second part is intended to provide temporary storage for dynamic resources.
    // Space for dynamic resources is allocated in chunks, and then descriptors are suballocated within every
    // chunk. DynamicSuballocationsManager facilitates this process.
    //
    //
    //     static and mutable handles      ||                 dynamic space
    //                                     ||    chunk 0     chunk 1     chunk 2     unused
    //  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||
    //                                               |         |
    //                                     suballocation       suballocation
    //                                    within chunk 0       within chunk 1
    //
    // Render device contains two GPUDescriptorHeap instances (CBV_SRV_UAV and SAMPLER). The heaps
    // are used to allocate GPU-visible descriptors for shader resource binding objects. The heaps
    // are also used by the command contexts (through DynamicSuballocationsManager to allocated dynamic descriptors)
    //
    //  _______________________________________________________________________________________________________________________________
    // | Render Device                                                                                                                 |
    // |                                                                                                                               |
    // | m_CPUDescriptorHeaps[CBV_SRV_UAV] |  X  X  X  X  X  X  X  X  |, |  X  X  X  X  X  X  X  X  |, |  X  O  O  X  O  O  O  O  |    |
    // | m_CPUDescriptorHeaps[SAMPLER]     |  X  X  X  X  O  O  O  X  |, |  X  O  O  X  O  O  O  O  |                                  |
    // | m_CPUDescriptorHeaps[RTV]         |  X  X  X  O  O  O  O  O  |, |  O  O  O  O  O  O  O  O  |                                  |
    // | m_CPUDescriptorHeaps[DSV]         |  X  X  X  O  X  O  X  O  |                                                                |
    // |                                                                               ctx1        ctx2                                |
    // | m_GPUDescriptorHeaps[CBV_SRV_UAV]  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||    |
    // | m_GPUDescriptorHeaps[SAMPLER]      | X X O O X O X X X O O X O O O O  ||  | X X O O | | X O O O | | O O O O |  O O O O  ||    |
    // |                                                                                                                               |
    // |_______________________________________________________________________________________________________________________________|
    //
    //  ________________________________________________ 
    // |Device Context 1                                |
    // |                                                |
    // | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |
    // | m_DynamicGPUDescriptorAllocator[SAMPLER]       |
    // |________________________________________________| 
    //
    export class GPUDescriptorHeap final : public IDescriptorAllocator
    {
    public:
        GPUDescriptorHeap(
            IRenderDevice& Device,
            uint32_t NumDescriptorsInHeap,
            uint32_t NumDynamicDescriptors,
            D3D12_DESCRIPTOR_HEAP_TYPE Type,
            D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
            :
            // clang-format off
            m_DeviceD3D12Impl{Device},
            m_HeapDesc
            {
                Type,
                NumDescriptorsInHeap + NumDynamicDescriptors,
                Flags,
                1 // UINT NodeMask;
            },
            m_pd3d12DescriptorHeap
            {
                [&]
                {
                    return Device.CreateDescriptorHeap(m_HeapDesc);
                }()
            },
            m_DescriptorSize{Device.GetDescriptorHandleIncrementSize(Type)},
            m_HeapAllocationManager{Device, *this, 0, m_pd3d12DescriptorHeap, 0, NumDescriptorsInHeap},
            m_DynamicAllocationsManager{Device, *this, 1, m_pd3d12DescriptorHeap, NumDescriptorsInHeap, NumDynamicDescriptors}
        // clang-format on
        {
        }

        // clang-format off
        GPUDescriptorHeap(const GPUDescriptorHeap&) = delete;
        GPUDescriptorHeap(GPUDescriptorHeap&&) = delete;
        GPUDescriptorHeap& operator =(const GPUDescriptorHeap&) = delete;
        GPUDescriptorHeap& operator =(GPUDescriptorHeap&&) = delete;
        // clang-format on

        ~GPUDescriptorHeap()
        {
            auto TotalStaticSize = m_HeapAllocationManager.GetMaxDescriptors();
            auto TotalDynamicSize = m_DynamicAllocationsManager.GetMaxDescriptors();
            auto MaxStaticSize = m_HeapAllocationManager.GetMaxAllocatedSize();
            auto MaxDynamicSize = m_DynamicAllocationsManager.GetMaxAllocatedSize();

            //LOG_INFO_MESSAGE(std::setw(38), std::left, GetD3D12DescriptorHeapTypeLiteralName(m_HeapDesc.Type),
            //    " GPU heap max allocated size (static|dynamic): ",
            //    MaxStaticSize, '/', TotalStaticSize, " (", std::fixed, std::setprecision(2), MaxStaticSize * 100.0 / TotalStaticSize,
            //    "%) | ",
            //    MaxDynamicSize, '/', TotalDynamicSize, " (", std::fixed, std::setprecision(2), MaxDynamicSize * 100.0 / TotalDynamicSize,
            //    "%).");
        }
        
        //todo: temp may be better with friend declaration
        std::shared_ptr<ID3D12DescriptorHeap> GetDescriptorHeap()
        {
            return m_pd3d12DescriptorHeap;
        }

        virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final
        {
            return m_HeapAllocationManager.Allocate(Count);
        }

        virtual void Free(DescriptorHeapAllocation&& Allocation, uint64_t CmdQueueMask) override final
        {
            struct StaleAllocation
            {
                DescriptorHeapAllocation Allocation;
                GPUDescriptorHeap* Heap;

                // clang-format off
                StaleAllocation(DescriptorHeapAllocation&& _Allocation, GPUDescriptorHeap& _Heap) noexcept :
                    Allocation{std::move(_Allocation)},
                    Heap{&_Heap}
                {
                }

                StaleAllocation(const StaleAllocation&) = delete;
                StaleAllocation& operator=(const StaleAllocation&) = delete;
                StaleAllocation& operator=(StaleAllocation&&) = delete;

                StaleAllocation(StaleAllocation&& rhs) noexcept :
                    Allocation{std::move(rhs.Allocation)},
                    Heap{rhs.Heap}
                {
                    rhs.Heap = nullptr;
                }

                // clang-format on

                ~StaleAllocation()
                {
                    if (Heap != nullptr)
                    {
                        auto MgrId = Allocation.GetAllocationManagerId();
                        assert(MgrId == 0 || MgrId == 1); //, "Unexpected allocation manager ID"

                        if (MgrId == 0)
                        {
                            Heap->m_HeapAllocationManager.FreeAllocation(std::move(Allocation));
                        }
                        else
                        {
                            Heap->m_DynamicAllocationsManager.FreeAllocation(std::move(Allocation));
                        }
                    }
                }
            };
            // todo review here should be enqueueing
            StaleAllocation{std::move(Allocation), *this};
            //m_DeviceD3D12Impl.SafeReleaseDeviceObject(StaleAllocation{std::move(Allocation), *this}, CmdQueueMask);
        }

        virtual uint32_t GetDescriptorSize() const override final { return m_DescriptorSize; }

        DescriptorHeapAllocation AllocateDynamic(uint32_t Count)
        {
            return m_DynamicAllocationsManager.Allocate(Count);
        }

        const D3D12_DESCRIPTOR_HEAP_DESC& GetHeapDesc() const { return m_HeapDesc; }
        uint32_t GetMaxStaticDescriptors() const { return m_HeapAllocationManager.GetMaxDescriptors(); }
        uint32_t GetMaxDynamicDescriptors() const { return m_DynamicAllocationsManager.GetMaxDescriptors(); }

#ifdef DILIGENT_DEVELOPMENT
    int32_t DvpGetTotalAllocationCount() const
    {
        return m_HeapAllocationManager.DvpGetAllocationsCounter() +
            m_DynamicAllocationsManager.DvpGetAllocationsCounter();
    }
#endif

    protected:
        IRenderDevice& m_DeviceD3D12Impl;

        const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
        std::shared_ptr<ID3D12DescriptorHeap> m_pd3d12DescriptorHeap; // Must be defined after m_HeapDesc

        const UINT m_DescriptorSize;

        // Allocation manager for static/mutable part
        DescriptorHeapAllocationManager m_HeapAllocationManager;

        // Allocation manager for dynamic part
        DescriptorHeapAllocationManager m_DynamicAllocationsManager;
    };


    // The class facilitates allocation of dynamic descriptor handles. It requests a chunk of heap
    // from the master GPU descriptor heap and then performs linear suballocation within the chunk
    // At the end of the frame all allocations are disposed.

    //     static and mutable handles     ||                 dynamic space
    //                                    ||    chunk 0                 chunk 2
    //  |                                 ||  | X X X O |             | O O O O |           || GPU Descriptor Heap
    //                                        |                       |
    //                                        m_Suballocations[0]     m_Suballocations[1]
    //
    export class DynamicSuballocationsManager final : public IDescriptorAllocator
    {
    public:
        DynamicSuballocationsManager(GPUDescriptorHeap& ParentGPUHeap,
            uint32_t DynamicChunkSize,
            std::string ManagerName)
            :
            // clang-format off
            m_ParentGPUHeap{ParentGPUHeap},
            m_ManagerName{std::move(ManagerName)},
            m_Suballocations(),
            m_DynamicChunkSize{DynamicChunkSize}
        // clang-format on
        {
        }

        // clang-format off
        DynamicSuballocationsManager(const DynamicSuballocationsManager&) = delete;
        DynamicSuballocationsManager(DynamicSuballocationsManager&&) = default;
        DynamicSuballocationsManager& operator =(const DynamicSuballocationsManager&) = delete;
        DynamicSuballocationsManager& operator =(DynamicSuballocationsManager&&) = delete;
        // clang-format on

        ~DynamicSuballocationsManager() override
        {
            assert(m_Suballocations.empty() && m_CurrDescriptorCount == 0 && m_CurrSuballocationsTotalSize == 0);
            // "All dynamic suballocations must be released!");
            //LOG_INFO_MESSAGE(m_ManagerName, " usage stats: peak descriptor count: ", m_PeakDescriptorCount, '/',
            //    m_PeakSuballocationsTotalSize);
        }

        void ReleaseAllocations(uint64_t CmdQueueMask)
        {
            // Clear the list and dispose all allocated chunks of GPU descriptor heap.
            // The chunks will be added to release queues and eventually returned to the
            // parent GPU heap.
            for (auto& Allocation : m_Suballocations)
            {
                m_ParentGPUHeap.Free(std::move(Allocation), CmdQueueMask);
                m_Suballocations.clear();
            }
            m_CurrDescriptorCount = 0;
            m_CurrSuballocationsTotalSize = 0;
        }

        virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final
        {
            // This method is intentionally lock-free as it is expected to
            // be called through device context from single thread only

            // Check if there are no chunks or the last chunk does not have enough space
            if (m_Suballocations.empty() ||
                size_t{m_CurrentSuballocationOffset} + size_t{Count} > m_Suballocations.back().GetNumHandles())
            {
                // Request a new chunk from the parent GPU descriptor heap
                auto SuballocationSize = std::max(m_DynamicChunkSize, Count);
                auto NewDynamicSubAllocation = m_ParentGPUHeap.AllocateDynamic(SuballocationSize);
                if (NewDynamicSubAllocation.IsNull())
                {
                    //LOG_ERROR("Dynamic space in ", GetD3D12DescriptorHeapTypeLiteralName(m_ParentGPUHeap.GetHeapDesc().Type),
                    //    " GPU descriptor heap is exhausted.");
                    return DescriptorHeapAllocation();
                }
                m_Suballocations.emplace_back(std::move(NewDynamicSubAllocation));
                m_CurrentSuballocationOffset = 0;

                m_CurrSuballocationsTotalSize += SuballocationSize;
                m_PeakSuballocationsTotalSize = std::max(m_PeakSuballocationsTotalSize, m_CurrSuballocationsTotalSize);
            }

            // Perform suballocation from the last chunk
            auto& CurrentSuballocation = m_Suballocations.back();

            auto ManagerId = CurrentSuballocation.GetAllocationManagerId();
            assert(ManagerId < std::numeric_limits<uint16_t>::max()); //, "ManagerID exceed allowed limit"

            DescriptorHeapAllocation Allocation(*this,
                CurrentSuballocation.GetDescriptorHeap(),
                CurrentSuballocation.GetCpuHandle(m_CurrentSuballocationOffset),
                CurrentSuballocation.GetGpuHandle(m_CurrentSuballocationOffset),
                Count,
                static_cast<uint16_t>(ManagerId));
            m_CurrentSuballocationOffset += Count;
            m_CurrDescriptorCount += Count;
            m_PeakDescriptorCount = std::max(m_PeakDescriptorCount, m_CurrDescriptorCount);

            return Allocation;
        }

        virtual void Free(DescriptorHeapAllocation&& Allocation, uint64_t CmdQueueMask) override final
        {
            // Do nothing. Dynamic allocations are not disposed individually, but as whole chunks
            // at the end of the frame by ReleaseAllocations()
            Allocation.Reset();
        }

        virtual uint32_t GetDescriptorSize() const override final { return m_ParentGPUHeap.GetDescriptorSize(); }

        size_t GetSuballocationCount() const { return m_Suballocations.size(); }

    private:
        // Parent GPU descriptor heap that is used to allocate chunks
        GPUDescriptorHeap& m_ParentGPUHeap;
        const std::string m_ManagerName;

        // List of chunks allocated from the master GPU descriptor heap. All chunks are disposed at the end
        // of the frame
        std::vector<DescriptorHeapAllocation> m_Suballocations;

        uint32_t m_CurrentSuballocationOffset = 0;
        uint32_t m_DynamicChunkSize = 0;

        uint32_t m_CurrDescriptorCount = 0;
        uint32_t m_PeakDescriptorCount = 0;
        uint32_t m_CurrSuballocationsTotalSize = 0;
        uint32_t m_PeakSuballocationsTotalSize = 0;
    };
}