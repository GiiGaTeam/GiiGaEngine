#pragma once

#include<functional>
#include<memory>
#include<unordered_set>

#include<Window.h>
#include<RenderDevice.h>
#include<RenderContext.h>
#include<DescriptorHeap.h>
#include<ShaderManager.h>
#include<IUpdateGPUData.h>
#include<CameraComponent.h>
#include <Core/Render/Nvrhi/NvrhiCommon.h>
#include <Core/Render/Nvrhi/NvrhiSwapChain.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiRenderPass.h>

namespace GiiGa
{
    // Unified NVRHI render system.
    //
    // Owns the D3D12 device (via RenderDevice, kept for descriptor-heap access),
    // a RenderContext (kept solely for its ID3D12CommandQueue, shared with NVRHI
    // and the DirectXTK12 ResourceUploadBatch used by asset loaders),
    // an rhi::SwapChain whose back buffers are NVRHI textures,
    // one nvrhi::CommandListHandle driven per-frame, and a rhi::RenderGraph.
    //
    // Frame loop (Tick):
    //   wait(frame-latency) -> beginFrame -> open(cmd) -> updaters -> execute(graph)
    //                       -> close(cmd) -> executeCommandList -> present -> runGarbageCollection
    class RenderSystem
    {
    public:
        RenderSystem(const RenderSystem& other) = delete;
        RenderSystem(RenderSystem&& other) noexcept = delete;
        RenderSystem& operator=(const RenderSystem& other) = delete;
        RenderSystem& operator=(RenderSystem&& other) noexcept = delete;

        virtual ~RenderSystem()
        {
            if (nvrhiDevice_) nvrhiDevice_->waitForIdle();
        }

        RenderSystem(Window& window):
            device_(),
            context_(device_)
        {
            // Wrap the same D3D12 device + graphics queue that RenderContext created.
            nvrhi::d3d12::DeviceDesc nd;
            nd.errorCB = &nvrhiMessages_;
            nd.pDevice = device_.GetDxDevice().get();
            nd.pGraphicsCommandQueue = context_.getGraphicsCommandQueue().get();
            nvrhiDevice_ = nvrhi::d3d12::createDevice(nd);

            // NVRHI-backed swap chain (replaces the legacy SwapChain).
            rhiSwapChain_ = std::make_unique<rhi::SwapChain>(
                nvrhiDevice_.Get(),
                context_.getGraphicsCommandQueue().get(),
                window.GetHandle(), 0, 0);

            // Single per-frame command list shared by every pass.
            cmd_ = nvrhiDevice_->createCommandList();
            frameCtx_.device = nvrhiDevice_.Get();
            frameCtx_.cmd = cmd_.Get();

            window.OnWindowResize.Register(std::bind(&RenderSystem::ResizeSwapChain, this, std::placeholders::_1));
        }

        virtual void SetCamera(std::shared_ptr<CameraComponent> camera) = 0;

        virtual void Initialize() = 0;

        virtual void Tick()
        {
            device_.DeleteStaleObjects();

            nvrhiDevice_->waitForIdle();

            if (HANDLE h = rhiSwapChain_->getFrameLatencyWaitableObject())
                WaitForSingleObject(h, INFINITE);

            rhiSwapChain_->beginFrame();

            cmd_->open();
            cmd_->clearState();

            for (auto it = updateGpuData_registry_.begin(); it != updateGpuData_registry_.end(); ++it)
            {
                it.operator*()->UpdateGPUData(nvrhiDevice_.Get(), cmd_.Get());
            }

            root_.execute(frameCtx_);

            cmd_->close();
            nvrhiDevice_->executeCommandList(cmd_.Get());
            rhiSwapChain_->present();
            nvrhiDevice_->runGarbageCollection();
        }

        RenderDevice& GetRenderDevice() { return device_; }

        nvrhi::IDevice* GetNvrhiDevice() { return nvrhiDevice_.Get(); }
        nvrhi::ICommandList* commandList() { return cmd_.Get(); }

        rhi::SwapChain& rhiSwapChain() { return *rhiSwapChain_; }
        rhi::FrameContext& frameContext() { return frameCtx_; }

        // Kept for asset loaders that use ResourceUploadBatch.
        RenderContext& GetRenderContext() { return context_; }

        // Legacy alias — the old upload command list is now the single per-frame command list.
        nvrhi::ICommandList* GetUploadCommandList() { return cmd_.Get(); }

        void ResizeSwapChain(const WindowResizeEvent& event)
        {
            nvrhiDevice_->waitForIdle();
            rhiSwapChain_->resize(event.width, event.height);
        }

        void RegisterInUpdateGPUData(IUpdateGPUData* data)
        {
            if (!updateGpuData_registry_.contains(data))
                updateGpuData_registry_.insert(data);
            else
                throw std::runtime_error("Update GPU data already exists");
        }

        void UnregisterInUpdateGPUData(IUpdateGPUData* data)
        {
            if (updateGpuData_registry_.contains(data))
                updateGpuData_registry_.erase(data);
            else
                throw std::runtime_error("Update GPU data not found");
        }

    protected:
        RenderDevice device_;
        RenderContext context_;  // kept for its ID3D12CommandQueue (shared with NVRHI + asset uploads)
        std::unordered_set<IUpdateGPUData*> updateGpuData_registry_;

        rhi::MessageCallback nvrhiMessages_;
        nvrhi::DeviceHandle nvrhiDevice_;
        nvrhi::CommandListHandle cmd_;

        std::unique_ptr<rhi::SwapChain> rhiSwapChain_;
        rhi::RenderGraph root_;
        rhi::FrameContext frameCtx_;
    };
}
