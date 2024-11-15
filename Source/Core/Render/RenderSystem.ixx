module;
#include <memory>

export module RenderSystem;

import RenderSystemSettings;
import Window;
import RenderDevice;
import RenderContext;

namespace GiiGa
{
    export class RenderSystem
    {
    public:
        void Init(Window window)
        {
            context_.Create(device_);
        }
    private:
        RenderDevice device_;
        RenderContext context_;
    };
}