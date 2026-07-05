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
            auto tempEditorSCP = std::make_shared<EditorSwapChainPass>(device_, &rhiSwapChain());
            editorSwapChainPass_ = tempEditorSCP;
            root_.addPass(tempEditorSCP);

            editorSwapChainPass_.lock()->viewports_.push_back(
                std::make_shared<EditorViewport>(device_, editorSwapChainPass_.lock()->editorContext_));
            for (auto& viewport : editorSwapChainPass_.lock()->viewports_)
            {
                viewport->Init();
            }
        }

        void SetCamera(std::shared_ptr<CameraComponent> camera) override
        {
            editorSwapChainPass_.lock()->viewports_[0]->SetCamera(camera);
        }

    private:
        std::weak_ptr<EditorSwapChainPass> editorSwapChainPass_;
    };
}
