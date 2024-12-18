module;

#include <imgui.h>
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

            root_.AddPass(std::make_shared<ShadowPass>());
            std::shared_ptr<EditorSwapChainPass> tempEditorSCP = std::make_shared<EditorSwapChainPass>(device_, swapChain_);
            editorSwapChainPass_ = tempEditorSCP;
            root_.AddPass(tempEditorSCP);
            
            editorSwapChainPass_.lock()->viewports_.push_back(std::make_shared<EditorViewport>(device_));
            for (auto& viewport : editorSwapChainPass_.lock()->viewports_)
            {
                viewport->Init();
            }
        }

    private:
        std::weak_ptr<EditorSwapChainPass> editorSwapChainPass_;
    };
}
