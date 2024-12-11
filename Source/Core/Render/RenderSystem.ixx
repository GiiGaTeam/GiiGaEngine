﻿module;

#include <functional>
#include <memory>
#include <iostream>

export module RenderSystem;

import RenderSystemSettings;
export import Window;
export import RenderDevice;
import RenderContext;
import DescriptorHeap;
import RenderPass;
import SwapChain;

namespace GiiGa
{
    export class RenderSystem
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
        }

        virtual void Initialize() = 0;

        virtual void Tick()
        {
            device_.DeleteStaleObjects();
            context_.StartFrame();
            root.Draw(context_);
            context_.EndFrame();
            swapChain_->Present();
        }
        RenderDevice& GetRenderDevice() {
            return device_;
        }

        RenderContext& GetRenderContext() {
            return context_;
        }

        void ResizeSwapChain(const WindowResizeEvent& event)
        {
            context_.ContextIdle();
            swapChain_->Resize(device_, event.width, event.height);
        }
    protected:
        RenderDevice device_;
        RenderContext context_;
        std::shared_ptr<SwapChain> swapChain_;
        RenderPass root;
    };
}
