module;

#include <memory>

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
        virtual ~RenderSystem() = default;

        RenderSystem(Window& window):
            device_(),
            context_(device_),
            swapChain_(std::make_shared<SwapChain>(device_, context_.getGraphicsCommandQueue(), window))
        {
            context_.SetFrameLatencyWaitableObject(swapChain_->GetFrameLatencyWaitableObject());
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

    protected:
        RenderDevice device_;
        RenderContext context_;
        std::shared_ptr<SwapChain> swapChain_;
        RenderPass root;
    };
}
