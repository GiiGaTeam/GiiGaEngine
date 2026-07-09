#pragma once

#include<functional>
#include<memory>
#include<unordered_set>

#include<Window.h>
#include<RenderDevice.h>
#include<RenderContext.h>
#include<RenderGraph.h>
#include<SwapChain.h>
#include<IUpdateGPUData.h>
#include<CameraComponent.h>

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
            device_.Device()->waitForIdle();
        }

        RenderSystem(Window& window):
            device_(),
            context_(device_),
            swapChain_(std::make_shared<SwapChain>(device_, window))
        {
            window.OnWindowResize.Register(std::bind(&RenderSystem::ResizeSwapChain, this, std::placeholders::_1));
        }

        virtual void SetCamera(std::shared_ptr<CameraComponent> camera) =0;

        virtual void Initialize() = 0;

        virtual void Tick()
        {
            context_.OpenFrame();

            for (auto it = updateGpuData_registry_.begin(); it != updateGpuData_registry_.end(); ++it)
            {
                (*it)->UpdateGPUData(context_);
            }

            root_.Draw(context_);

            context_.CloseAndExecute();
            swapChain_->Present();

            device_.Device()->runGarbageCollection();
        }

        RenderDevice& GetRenderDevice() { return device_; }

        RenderContext& GetRenderContext() { return context_; }

        void ResizeSwapChain(const WindowResizeEvent& event)
        {
            device_.Device()->waitForIdle();
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
    };
}
