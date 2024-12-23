#include<directxtk12/SimpleMath.h>

export module EditorViewport;

import <imgui.h>;
import <string>;
import <memory>;
import <d3d12.h>;
import <iostream>;

export import Viewport;
import RenderDevice;
import ForwardPass;
import RenderGraph;
import World;
import CameraComponent;
import GameObject;
import SpectatorMovementComponent;

namespace GiiGa
{
    export class EditorViewport : public Viewport, public std::enable_shared_from_this<EditorViewport>
    {
    public:
        EditorViewport(RenderDevice& device):
            Viewport(device)
        {
            Resize(viewport_size_);
        }

        void Init() override
        {
            renderGraph_ = std::make_shared<RenderGraph>();
            renderGraph_->AddPass(std::make_shared<ForwardPass>(std::bind(&EditorViewport::GetCameraInfo, this)));

            camera_ = GameObject::CreateEmptyGameObject({.name = "Viewport Camera"});
            const auto cameraComponent = camera_.lock()->CreateComponent<CameraComponent>(Perspective, 90, 16 / 9);
            camera_.lock()->CreateComponent<SpectatorMovementComponent>();
        }

        RenderPassViewMatricies GetCameraInfo() override
        {
            auto camera_go = camera_.lock();

            // do we need view mat in camera?
            auto camera = camera_go->GetComponent<CameraComponent>()->GetCamera();

            return {.viewMatrix = camera.view_, .projMatrix = camera.GetProj()};
        }

        void Execute(RenderContext& context) override
        {
            if (camera_.expired()) return;

            ImGui::Begin(("Viewport" + std::to_string(viewport_index)).c_str());

            auto current_size = ImGui::GetWindowSize();

            if (current_size.x != viewport_size_.x || current_size.y != viewport_size_.y)
                Resize({current_size.x, current_size.y});

            resultResource_->StateTransition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);

            const float clear_color_with_alpha[4] = {1, 0, 0, 1};

            const auto RTHandle = resultRTV_->getDescriptor().GetCpuHandle();
            const auto RTHandle2 = resultRTV_->getDescriptor().GetGpuHandle();
            context.ClearRenderTargetView(RTHandle, clear_color_with_alpha);
            context.GetGraphicsCommandList()->OMSetRenderTargets(1, &RTHandle, true, nullptr);
            // draw here
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = current_size.x;
            viewport.Height = current_size.y;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            context.GetGraphicsCommandList()->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = {};
            scissorRect.left = 0;
            scissorRect.top = 0;
            scissorRect.right = current_size.x;
            scissorRect.bottom = current_size.y;
            context.GetGraphicsCommandList()->RSSetScissorRects(1, &scissorRect);

            renderGraph_->Draw(context);
            // draw here - end

            resultResource_->StateTransition(context, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            //ImGui::Image((ImTextureID)RTHandle.ptr, ImVec2(viewport_size_.x, viewport_size_.y));
            ImGui::Image((ImTextureID)resultSRV_->getDescriptor().getGPUHandle().ptr, ImVec2(viewport_size_.x, viewport_size_.y));

            ImGui::End();
        }

    private:
        int viewport_index = 0;
        std::weak_ptr<GameObject> camera_;
    };
}
