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
            std::shared_ptr<EditorSwapChainPass> tempEditorSCP = std::make_shared<EditorSwapChainPass>(device_, swapChain_);
            editorSwapChainPass_ = tempEditorSCP;
            root.AddChild(tempEditorSCP);

            editorSwapChainPass_.lock()->viewports_.push_back(std::make_shared<EditorViewport>(device_));
        }

    private:
        std::weak_ptr<EditorSwapChainPass> editorSwapChainPass_;
    };
}