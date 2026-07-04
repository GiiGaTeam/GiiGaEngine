#pragma once

#include<functional>
#include<memory>
#include<unordered_set>

#include<Window.h>
#include<RenderDevice.h>
#include<RenderContext.h>
#include<DescriptorHeap.h>
#include<RenderGraph.h>
#include<SwapChain.h>
#include<ShaderManager.h>
#include<Viewport.h>
#include<IUpdateGPUData.h>
#include<CameraComponent.h>
#include <Core/Render/Nvrhi/NvrhiCommon.h>

namespace GiiGa
{
    class RenderSystem
    {
    public:
        RenderSystem(const RenderSystem& other) = delete;
        RenderSystem(RenderSystem&& other) noexcept = delete;
        RenderSystem& operator=(const RenderSystem& other) = delete;
        RenderSystem& operator=(RenderSystem&& other) noexcept = delete;

        virtual ~RenderSystem()
        {
            context_.ContextIdle();
        }

        RenderSystem(Window& window):
            device_(),
            context_(device_),
            swapChain_(std::make_shared<SwapChain>(device_, context_.getGraphicsCommandQueue(), window))
        {
            context_.SetFrameLatencyWaitableObject(swapChain_->GetFrameLatencyWaitableObject());
            window.OnWindowResize.Register(std::bind(&RenderSystem::ResizeSwapChain, this, std::placeholders::_1));
            //shaderManager_ = std::make_unique<ShaderManager>();

            // NVRHI: wrap the existing D3D12 device + graphics queue so resource creation
            // (textures, buffers) can move to NVRHI while the legacy frame loop is still in place.
            nvrhi::d3d12::DeviceDesc nd;
            nd.errorCB = &nvrhiMessages_;
            nd.pDevice = device_.GetDxDevice().get();
            nd.pGraphicsCommandQueue = context_.getGraphicsCommandQueue().get();
            nvrhiDevice_ = nvrhi::d3d12::createDevice(nd);
            uploadCmd_ = nvrhiDevice_->createCommandList();
        }

        virtual void SetCamera(std::shared_ptr<CameraComponent> camera) =0;

        virtual void Initialize() = 0;

        virtual void Tick()
        {
            device_.DeleteStaleObjects();
            context_.StartFrame();

            // NVRHI cutover: per-frame GPU data is pushed through the NVRHI device + command list.
            // (Transitional — the legacy frame loop below is replaced once passes are converted.)
            for (auto it = updateGpuData_registry_.begin(); it != updateGpuData_registry_.end(); ++it)
            {
                it.operator*()->UpdateGPUData(nvrhiDevice_.Get(), uploadCmd_.Get());
            }

            root_.Draw(context_);
            context_.EndFrame();
            swapChain_->Present();
        }

        RenderDevice& GetRenderDevice()
        {
            return device_;
        }

        // NVRHI device wrapping the same D3D12 device — use for creating NVRHI resources.
        nvrhi::IDevice* GetNvrhiDevice() { return nvrhiDevice_.Get(); }
        // A command list for one-shot uploads (open/close/execute around use).
        nvrhi::ICommandList* GetUploadCommandList() { return uploadCmd_.Get(); }

        RenderContext& GetRenderContext()
        {
            return context_;
        }

        void ResizeSwapChain(const WindowResizeEvent& event)
        {
            context_.ContextIdle();
            swapChain_->Resize(device_, event.width, event.height);
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
        RenderContext context_;
        std::unordered_set<IUpdateGPUData*> updateGpuData_registry_;
        std::shared_ptr<SwapChain> swapChain_;
        RenderGraph root_;
        std::unique_ptr<ShaderManager> shaderManager_;

        rhi::MessageCallback nvrhiMessages_;
        nvrhi::DeviceHandle nvrhiDevice_;
        nvrhi::CommandListHandle uploadCmd_;
    };
}

//template <>
//struct std::hash<std::shared_ptr<GiiGa::IUpdateGPUData>>
//{
//    size_t operator()(std::shared_ptr<GiiGa::IUpdateGPUData> s_ptr) const noexcept
//    {
//        return std::hash<GiiGa::IUpdateGPUData*>()(s_ptr.get());
//    }
//};
