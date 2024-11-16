module;


export module RenderSystem;

import RenderSystemSettings;
import Window;
import RenderDevice;
import RenderContext;
import SwapChain;
import DescriptorHeap;

namespace GiiGa
{
    export class RenderSystem
    {
    public:
        RenderSystem(Window& window):
            device_(), context_(device_), swap_chain_(context_.getGraphicsCommandQueue(), window)
        {
            context_.SetSwapChainWaitable(swap_chain_.GetFrameLatencyWaitableObject());
        }

        void Tick()
        {
            
        }

    private:
        RenderDevice device_;
        RenderContext context_;
        SwapChain swap_chain_;
    };
}