module;


export module RenderSystem;

import RenderSystemSettings;
import Window;
import RenderDevice;
import RenderContext;
import DescriptorHeap;

namespace GiiGa
{
    export class RenderSystem
    {
    public:
        RenderSystem(Window& window):
            device_(), context_(device_,window)
        {
            
        }

        void Tick()
        {
            context_.Tick();
        }

    private:
        RenderDevice device_;
        RenderContext context_;
    };
}