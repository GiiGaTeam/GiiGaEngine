#include <imgui_internal.h>
#include<directxtk12/SimpleMath.h>

export module EditorViewport;

import <imgui.h>;
import <string>;
import <memory>;
import <d3d12.h>;
import <iostream>;

export import Viewport;
import RenderDevice;
import GBufferPass;
import GLightPass;
import RenderGraph;
import World;
import CameraComponent;
import GameObject;
import SpectatorMovementComponent;
import GBuffer;
import Input;
import Logger;

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

        void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            Viewport::Resize(new_size);

            if (auto l_camera = camera_.lock())
            {
                l_camera->GetComponent<CameraComponent>()->SetAspect(new_size.x/new_size.y);
            }
        }

        void Init(RenderContext& context) override
        {
            renderGraph_ = std::make_shared<RenderGraph>();

            renderGraph_->AddPass(std::make_shared<GBufferPass>(context, std::bind(&EditorViewport::GetCameraInfo, this), gbuffer_));
            renderGraph_->AddPass(std::make_shared<GLightPass>(context, std::bind(&EditorViewport::GetCameraInfo, this), gbuffer_));

            camera_ = GameObject::CreateEmptyGameObject({.name = "Viewport Camera"});
            const auto cameraComponent = camera_.lock()->CreateComponent<CameraComponent>(Perspective, 90, 16 / 9);
            camera_.lock()->CreateComponent<SpectatorMovementComponent>();
        }

        RenderPassViewData GetCameraInfo() override
        {
            return RenderPassViewData{.ViewProjMat = ViewProjectionMatrix, .viewDescriptor = ViewInfoConstBuffer->getDescriptor().getGPUHandle()};
        }

        void Execute(RenderContext& context) override
        {
            if (camera_.expired()) return;
            if (const auto movement = camera_.lock()->GetComponent<SpectatorMovementComponent>())
            {
                movement->active_ = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            }

            UpdateCameraInfo(context);

            ImGui::Begin(("Viewport" + std::to_string(viewport_index)).c_str());


            auto current_size = ImGui::GetWindowSize();

            if (current_size.x != viewport_size_.x || current_size.y != viewport_size_.y)
                Resize({current_size.x, current_size.y});

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

            context.GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            renderGraph_->Draw(context);
            // draw here - end

            gbuffer_->TransitionResource(context, GBuffer::GBufferOrder::LightAccumulation, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            //ImGui::Image((ImTextureID)RTHandle.ptr, ImVec2(viewport_size_.x, viewport_size_.y));
            ImGui::Image((ImTextureID)gbuffer_->GetSRV(GBuffer::GBufferOrder::LightAccumulation).ptr, ImVec2(viewport_size_.x, viewport_size_.y));

            ImGui::End();
        }

    private:
        int viewport_index = 0;
        std::weak_ptr<GameObject> camera_;
        std::shared_ptr<BufferView<Constant>> ViewInfoConstBuffer;
        DirectX::SimpleMath::Matrix ViewProjectionMatrix;

        void UpdateCameraInfo(RenderContext& context)
        {
            auto camera_go = camera_.lock();
            auto camera = camera_go->GetComponent<CameraComponent>()->GetCamera();

            ViewProjectionMatrix = camera.GetViewProj();

            RenderPassViewMatricies const_buf_data = {camera.view_.Transpose(), camera.GetProj().Transpose()};

            const auto CameraMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&const_buf_data), sizeof(RenderPassViewMatricies)};

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, sizeof(RenderPassViewMatricies));

            ViewInfoConstBuffer = context.AllocateDynamicConstantView(CameraMatricesSpan, desc);
        }
    };
}
