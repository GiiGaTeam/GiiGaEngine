module;
#include <d3d12.h>
#include <memory>

export module RenderDevice;
import DirectXUtils;

namespace GiiGa
{
export class RenderDevice
{
public:
    void CreateDevice()
    {
        ID3D12Device* device;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&device)) !=
            S_OK)
            return;
        device_ = std::shared_ptr<ID3D12Device>(device, DirectXDeleter());
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

    std::shared_ptr<ID3D12Device> device_;
};
}
