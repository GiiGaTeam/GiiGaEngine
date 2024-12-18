module;

#include <imgui.h>
#include <memory>

export module EditorRenderSystem;

import RenderSystem;
import ShadowPass;
import EditorSwapChainPass;
import EditorViewport;
import World;
import CameraComponent;
import GameObject;
import SpectatorMovementComponent;

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

            
            const auto editorCamera = World::CreateGameObject();
            const auto cameraComponent = editorCamera->CreateComponent<CameraComponent>(Perspective, 90, 16/9);
            editorCamera->CreateComponent<SpectatorMovementComponent>();
            
            editorCamera->Init();
            editorSwapChainPass_.lock()->viewports_.push_back(std::make_shared<EditorViewport>(device_, cameraComponent));
            for (auto& viewport : editorSwapChainPass_.lock()->viewports_)
            {
                viewport->Init();
            }
        }

    private:
        std::weak_ptr<EditorSwapChainPass> editorSwapChainPass_;
    };
}
