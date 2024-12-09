module;

#include <memory>

export module EditorRenderSystem;

import RenderSystem;
import ShadowPass;
import EditorSwapChainPass;
import EditorViewport;

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
            
            root.AddChild(std::make_shared<ShadowPass>());
            editorSwapChainPass_ = std::make_shared<EditorSwapChainPass>(device_, swapChain_);
            root.AddChild(editorSwapChainPass_);
            
            editorSwapChainPass_->viewports_.push_back(std::make_shared<EditorViewport>(device_));
        }
    private:
        std::shared_ptr<EditorSwapChainPass> editorSwapChainPass_;
    };
}
