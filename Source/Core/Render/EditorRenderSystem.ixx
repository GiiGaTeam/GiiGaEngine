module;

#include <memory>

export module EditorRenderSystem;

import RenderSystem;
import ShadowPass;
import EditorSwapChainPass;

namespace GiiGa 
{
    export class EditorRenderSystem : public RenderSystem 
    {
    public:
        explicit EditorRenderSystem(Window& window)
            : RenderSystem(window)
        {
        }

        void Initialize() override
        {
            //RenderSystem::Initialize();
            
            root.AddChild(std::make_unique<ShadowPass>());
            root.AddChild(std::make_unique<EditorSwapChainPass>(device_, swapChain_));
        }
    };
}
