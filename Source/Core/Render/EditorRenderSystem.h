#pragma once


#include<memory>

#include<RenderSystem.h>
#include<ShadowPass.h>
#include<EditorSwapChainPass.h>
#include<EditorViewport.h>

namespace GiiGa
{
    class EditorRenderSystem : public RenderSystem
    {
    public:
        explicit EditorRenderSystem(Window& window)
            : RenderSystem(window)
        {
        }

        void Initialize() override
        {
            //RenderSystem::Initialize();
            std::shared_ptr<EditorSwapChainPass> tempEditorSCP = std::make_shared<EditorSwapChainPass>(device_, swapChain_);
            editorSwapChainPass_ = tempEditorSCP;
            root_.AddPass(tempEditorSCP);
            
            editorSwapChainPass_.lock()->viewports_.push_back(std::make_shared<EditorViewport>(device_, editorSwapChainPass_.lock()->editorContext_));
            for (auto& viewport : editorSwapChainPass_.lock()->viewports_)
            {
                viewport->Init(context_);
            }
        }

    private:
        std::weak_ptr<EditorSwapChainPass> editorSwapChainPass_;
    };
}
