module;

#include<memory>
#include<deque>
#include<d3d12.h>
#include<directx/d3dx12.h>

export module UploadBuffer;

import Align;
import RenderDevice;

namespace GiiGa
{
    class UploadBuffer
    {
    public:
        // Use to upload data to the GPU
        struct Allocation
        {
            void* CPU;
            D3D12_GPU_VIRTUAL_ADDRESS GPU;
        };

        /**
         * The maximum size of an allocation is the size of a single page.
         */
        size_t GetPageSize() const
        {
            return m_PageSize;
        }

        /**
         * Allocate memory in an Upload heap.
         * An allocation must not exceed the size of a page.
         * Use a memcpy or similar method to copy the
         * buffer data to CPU pointer in the Allocation structure returned from
         * this function.
         */
        Allocation Allocate(size_t sizeInBytes, size_t alignment)
        {
            if (sizeInBytes > m_PageSize)
            {
                throw std::bad_alloc();
            }

            // If there is no current page, or the requested allocation exceeds the
            // remaining space in the current page, request a new page.
            if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
            {
                m_CurrentPage = RequestPage();
            }

            return m_CurrentPage->Allocate(sizeInBytes, alignment);
        }

        /**
         * Release all allocated pages. This should only be done when the command list
         * is finished executing on the CommandQueue.
         */
        void Reset()
        {
            m_CurrentPage = nullptr;
            // Reset all available pages.
            m_AvailablePages = m_PagePool;

            for (auto page : m_AvailablePages)
            {
                // Reset the page for new allocations.
                page->Reset();
            }
        }

    protected:
        friend class std::default_delete<UploadBuffer>;

        /**
         * @param pageSize The size to use to allocate new pages in GPU memory.
         */
        explicit UploadBuffer(RenderDevice& device, size_t pageSize = 2 * 1024 * 1024)
            : m_Device(device)
              , m_PageSize(pageSize)
        {
        }

        virtual ~UploadBuffer()
        {
        }

    private:
        // A single page for the allocator.
        struct Page
        {
            Page(RenderDevice& device, size_t sizeInBytes)
                : m_Device(device)
                  , m_PageSize(sizeInBytes)
                  , m_Offset(0)
                  , m_CPUPtr(nullptr)
                  , m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
            {
                m_d3d12Resource = m_Device.CreateCommittedResource(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                    CD3DX12_RESOURCE_DESC::Buffer(m_PageSize), D3D12_RESOURCE_STATE_GENERIC_READ);

                m_d3d12Resource->SetName(L"Upload Buffer (Page)");

                m_GPUPtr = m_d3d12Resource->GetGPUVirtualAddress();
                m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
            }

            ~Page()
            {
                m_d3d12Resource->Unmap(0, nullptr);
                m_CPUPtr = nullptr;
                m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
            }

            // Check to see if the page has room to satisfy the requested
            // allocation.
            bool HasSpace(size_t sizeInBytes, size_t alignment) const
            {
                size_t alignedSize = AlignUp(sizeInBytes, alignment);
                size_t alignedOffset = AlignUp(m_Offset, alignment);

                return alignedOffset + alignedSize <= m_PageSize;
            }

            // Allocate memory from the page.
            // Throws std::bad_alloc if the the allocation size is larger
            // that the page size or the size of the allocation exceeds the
            // remaining space in the page.
            Allocation Allocate(size_t sizeInBytes, size_t alignment)
            {
                if (!HasSpace(sizeInBytes, alignment))
                {
                    // Can't allocate space from page.
                    throw std::bad_alloc();
                }

                size_t alignedSize = AlignUp(sizeInBytes, alignment);
                m_Offset = AlignUp(m_Offset, alignment);

                Allocation allocation;
                allocation.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
                allocation.GPU = m_GPUPtr + m_Offset;

                m_Offset += alignedSize;

                return allocation;
            }

            // Reset the page for reuse.
            void Reset()
            {
                m_Offset = 0;
            }

        private:
            RenderDevice& m_Device;
            std::shared_ptr<ID3D12Resource> m_d3d12Resource;

            // Base pointer.
            void* m_CPUPtr;
            D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

            // Allocated page size.
            size_t m_PageSize;
            // Current allocation offset in bytes.
            size_t m_Offset;
        };

        // A pool of memory pages.
        using PagePool = std::deque<std::shared_ptr<Page>>;

        // The device that was used to create this upload buffer.
        RenderDevice& m_Device;

        // Request a page from the pool of available pages
        // or create a new page if there are no available pages.
        std::shared_ptr<Page> RequestPage()
        {
            std::shared_ptr<Page> page;

            if (!m_AvailablePages.empty())
            {
                page = m_AvailablePages.front();
                m_AvailablePages.pop_front();
            }
            else
            {
                page = std::make_shared<Page>(m_Device, m_PageSize);
                m_PagePool.push_back(page);
            }

            return page;
        }

        PagePool m_PagePool;
        PagePool m_AvailablePages;

        std::shared_ptr<Page> m_CurrentPage;

        // The size of each page of memory.
        size_t m_PageSize;
    };
}