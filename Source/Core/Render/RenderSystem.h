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

namespace GiiGa
{
    class RenderSystem
    {
    public:
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
        }

        virtual void Initialize() = 0;

        virtual void Tick()
        {
            device_.DeleteStaleObjects();
            context_.StartFrame();

            for (auto it = updateGpuData_registry_.begin(); it != updateGpuData_registry_.end(); ++it)
            {
                it.operator*()->UpdateGPUData(context_);
            }

            root_.Draw(context_);
            context_.EndFrame();
            swapChain_->Present();
        }

        RenderDevice& GetRenderDevice()
        {
            return device_;
        }

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
