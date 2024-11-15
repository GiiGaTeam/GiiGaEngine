module;

#include <memory>
#include <queue>
#include <assert.h>
#include <iostream>

#include "d3d12.h"
export module CommandQueue;
import RenderDevice;

namespace GiiGa
{

export class CommandQueue final
{
public:
    CommandQueue(RenderDevice device, D3D12_COMMAND_LIST_TYPE type)
        : fence_value_(0)
          , command_list_type_(type)
          , device_(device)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        command_queue_ = device_.CreateCommandQueue(desc);
        fence_ = device_.CreateFence(fence_value_, D3D12_FENCE_FLAG_NONE);

        fence_event_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(fence_event_ && "Failed to create fence event handle.");
    }

    // Get an available command list from the command queue.
    std::shared_ptr<ID3D12GraphicsCommandList2> GetCommandList()
    {
        std::shared_ptr<ID3D12CommandAllocator> commandAllocator;
        std::shared_ptr<ID3D12GraphicsCommandList2> commandList;

        if (!m_command_allocator_queue_.empty() && IsFenceComplete(m_command_allocator_queue_.front().fence_value))
        {
            commandAllocator = m_command_allocator_queue_.front().commandAllocator;
            m_command_allocator_queue_.pop();

            if (commandAllocator->Reset() != S_OK) return nullptr;
        }
        else
        {
            commandAllocator = CreateCommandAllocator();
        }

        if (!m_command_list_queue_.empty())
        {
            commandList = m_command_list_queue_.front();
            m_command_list_queue_.pop();

            if (commandList->Reset(commandAllocator.Get(), nullptr)) return nullptr;
        }
        else
        {
            commandList = CreateCommandList(commandAllocator);
        }

        // Associate the command allocator with the command list so that it can be
        // retrieved when the command list is executed.
        if (commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get())) return nullptr;

        return commandList;
    }

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    uint64_t ExecuteCommandList(std::shared_ptr<ID3D12GraphicsCommandList2> commandList)
    {
        commandList->Close();

        ID3D12CommandAllocator* commandAllocator;
        UINT dataSize = sizeof(commandAllocator);
        ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

        ID3D12CommandList* const ppCommandLists[] = {
            commandList.Get()
        };

        command_queue_->ExecuteCommandLists(1, ppCommandLists);
        uint64_t fenceValue = Signal();

        m_command_allocator_queue_.emplace(CommandAllocatorEntry{fenceValue, commandAllocator});
        m_command_list_queue_.push(commandList);

        // The ownership of the command allocator has been transferred to the ComPtr
        // in the command allocator queue. It is safe to release the reference 
        // in this temporary COM pointer here.
        commandAllocator->Release();

        return fenceValue;
    }


    std::shared_ptr<CommandList> CommandQueue::GetCommandList()
    {
        std::shared_ptr<CommandList> commandList;

        // If there is a command list on the queue.
        if (!m_AvailableCommandLists.Empty())
        {
            m_AvailableCommandLists.TryPop(commandList);
        }
        else
        {
            // Otherwise create a new command list.
            commandList = std::make_shared<MakeCommandList>(m_Device, m_CommandListType);
        }

        return commandList;
    }

    uint64_t Signal()
    {
        uint64_t fenceValue = ++fence_value_;
        command_queue_->Signal(fence_.Get(), fenceValue);
        return fenceValue;
    }

    bool IsFenceComplete(uint64_t fenceValue_)
    {
        return fence_->GetCompletedValue() >= fenceValue;
    }

    void WaitForFenceValue(uint64_t fenceValue_)
    {
        if (!IsFenceComplete(fenceValue))
        {
            fence_->SetEventOnCompletion(fenceValue, fence_event_);
            ::WaitForSingleObject(fence_event_, DWORD_MAX);
        }
    }

    void Flush()
    {
        WaitForFenceValue(Signal());
    }


    std::shared_ptr<ID3D12CommandQueue> GetD3D12CommandQueue() const
    {
        return command_queue_;
    }

protected:
    std::shared_ptr<ID3D12CommandAllocator> CreateCommandAllocator()
    {
        std::shared_ptr<ID3D12CommandAllocator> commandAllocator;
        ThrowIfFailed(device_->CreateCommandAllocator(command_list_type_, IID_PPV_ARGS(&commandAllocator)));

        return commandAllocator;
    }

    std::shared_ptr<ID3D12GraphicsCommandList2> CreateCommandList(std::shared_ptr<ID3D12CommandAllocator> allocator)
    {
        std::shared_ptr<ID3D12GraphicsCommandList2> commandList;
        ThrowIfFailed(device_->CreateCommandList(0, command_list_type_, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

        return commandList;
    }

private:
    // Keep track of command allocators that are "in-flight"
    struct CommandAllocatorEntry
    {
        uint64_t fence_value;
        std::shared_ptr<ID3D12CommandAllocator> command_allocator;
    };

    using CommandAllocatorArray = std::array<CommandAllocatorEntry>;
    using CommandListArray = std::array<std::shared_ptr<ID3D12GraphicsCommandList2>>;


    D3D12_COMMAND_LIST_TYPE command_list_type_;
    RenderDevice device_;
    std::shared_ptr<ID3D12CommandQueue> command_queue_;
    std::shared_ptr<ID3D12Fence> fence_;
    HANDLE fence_event_;
    uint64_t fence_value_;

    CommandAllocatorArray m_command_allocator_queue_;
    CommandListArray m_command_list_queue_;
};
};
