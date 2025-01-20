#pragma once
#include <pybind11/embed.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include <ImGuizmo.h>


#include<imgui.h>
#include<string>
#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>

#include<Viewport.h>
#include<RenderDevice.h>
#include<GBufferPass.h>
#include<GLightPass.h>
#include<RenderGraph.h>
#include"World.h"
#include<CameraComponent.h>
#include<GameObject.h>
#include<SpectatorMovementComponent.h>
#include<GBuffer.h>
#include<EditorContext.h>
#include<ShadowPass.h>
#include<PostProcessPass.h>

#include "DebugPass.h"
#include "ForwardPass.h"

namespace GiiGa
{
    class EditorViewport : public Viewport, public std::enable_shared_from_this<EditorViewport>
    {
    public:
        EditorViewport(RenderDevice& device, std::shared_ptr<EditorContext> editor_ctx):
            Viewport(device)
            , editorContext_(editor_ctx)
        {
        }

        void Resize(DirectX::SimpleMath::Vector2 new_size) override
        {
            Viewport::Resize(new_size);

            if (auto l_camera = camera_.lock())
            {
                l_camera->GetComponent<CameraComponent>()->SetAspect(new_size.x / new_size.y);
            }
        }

        void Init(RenderContext& context) override
        {
            Resize(viewport_size_);

            renderGraph_ = std::make_shared<RenderGraph>();

            renderGraph_->AddPass(std::make_shared<ShadowPass>(context, std::bind(&EditorViewport::GetCameraInfo, this)));
            renderGraph_->AddPass(std::make_shared<GBufferPass>(context, std::bind(&EditorViewport::GetCameraInfo, this), gbuffer_));
            renderGraph_->AddPass(std::make_shared<GLightPass>(context, std::bind(&EditorViewport::GetCameraInfo, this), gbuffer_));
            renderGraph_->AddPass(std::make_shared<ForwardPass>(context, std::bind(&EditorViewport::GetCameraInfo, this)));
            renderGraph_->AddPass(std::make_shared<DebugPass>(context, std::bind(&EditorViewport::GetCameraInfo, this)));
            renderGraph_->AddPass(std::make_shared<PostProcessPass>(context, std::bind(&EditorViewport::GetResultInfo, this), gbuffer_));

            camera_ = GameObject::CreateEmptyGameObject({.name = "Viewport Camera"});
            const auto cameraComponent = camera_.lock()->CreateComponent<CameraComponent>(Perspective, 90, 16 / 9);
            camera_.lock()->CreateComponent<SpectatorMovementComponent>();
        }

        RenderPassViewData GetCameraInfo() override
        {
            return RenderPassViewData{
                camera_.lock()->GetComponent<CameraComponent>()->GetCamera(), viewInfoConstBuffer->getDescriptor().getGPUHandle(),
                viewport_size_, screenDimensionsConstBuffer->getDescriptor().getGPUHandle()
            };
        }

        void Execute(RenderContext& context) override
        {
            // TODO: Check simulation mode or editor mode

            if (camera_.expired()) return;

            if (ImGui::Begin(("Viewport" + std::to_string(viewport_index)).c_str()))
            {
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                {
                    if (const auto movement = camera_.lock()->GetComponent<SpectatorMovementComponent>())
                    {
                        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 5, ImGui::GetCursorPos().y));
                        movement->active_ = ImGui::IsMouseDown(ImGuiMouseButton_Right);
                    }
                }
            }

            UpdateCameraInfo(context);

            auto current_size = ImGui::GetContentRegionAvail();
            if (current_size.y < 0)
            {
                current_size.y = 2; // Или skip
                ImGui::Text("Content height is negative!");
            }

            if (current_size.x != viewport_size_.x || current_size.y != viewport_size_.y)
                Resize({current_size.x, current_size.y});

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::AllowAxisFlip(false);
            ImGuizmo::SetDrawlist();

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

            rtvData_.resource_->StateTransition(context, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            //ImGui::Image((ImTextureID)RTHandle.ptr, ImVec2(viewport_size_.x, viewport_size_.y));
            ImGui::Image((ImTextureID)rtvData_.SRV_->getDescriptor().getGPUHandle().ptr, ImVec2(viewport_size_.x, viewport_size_.y));

            // Widgets on viewport
            PostViewportDrawWidgets();

            ImGui::End();
        }

    private:
        int viewport_index = 0;
        std::weak_ptr<GameObject> camera_;
        std::shared_ptr<BufferView<Constant>> viewInfoConstBuffer;
        std::shared_ptr<BufferView<Constant>> screenDimensionsConstBuffer;
        DirectX::SimpleMath::Matrix viewProjectionMatrix;

        std::shared_ptr<EditorContext> editorContext_;

        void DrawToolbar()
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
                editorContext_->currentOperation_ = ImGuizmo::OPERATION::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
                editorContext_->currentOperation_ = ImGuizmo::OPERATION::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
                editorContext_->currentOperation_ = ImGuizmo::OPERATION::SCALEU;

            const float header_h = ImGui::GetTextLineHeightWithSpacing();

            ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + 10.0f, ImGui::GetWindowPos().y + header_h + 10.0f));

            ImGui::BeginChild("ToolBar");

            bool translate = editorContext_->currentOperation_ & ImGuizmo::OPERATION::TRANSLATE;
            bool rotate = editorContext_->currentOperation_ & ImGuizmo::OPERATION::ROTATE;
            bool scale = editorContext_->currentOperation_ & ImGuizmo::OPERATION::SCALEU;

            if (ImGui::Checkbox("Translate", &translate))
            {
                editorContext_->currentOperation_ = static_cast<ImGuizmo::OPERATION>(translate ? editorContext_->currentOperation_ | ImGuizmo::OPERATION::TRANSLATE : editorContext_->currentOperation_ & ~ImGuizmo::OPERATION::TRANSLATE);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hotkey: W");
            }
            ImGui::SameLine();

            if (ImGui::Checkbox("Rotate", &rotate))
            {
                editorContext_->currentOperation_ = static_cast<ImGuizmo::OPERATION>(rotate ? editorContext_->currentOperation_ | ImGuizmo::OPERATION::ROTATE : editorContext_->currentOperation_ & ~ImGuizmo::OPERATION::ROTATE);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hotkey: E");
            }
            ImGui::SameLine();

            if (ImGui::Checkbox("Scale", &scale))
            {
                editorContext_->currentOperation_ = static_cast<ImGuizmo::OPERATION>(scale ? editorContext_->currentOperation_ | ImGuizmo::OPERATION::SCALEU : editorContext_->currentOperation_ & ~ImGuizmo::OPERATION::SCALEU);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hotkey: S");
            }
            ImGui::SameLine();

            if (ImGui::RadioButton("World", editorContext_->currentMode_ == ImGuizmo::MODE::WORLD))
            {
                editorContext_->currentMode_ = ImGuizmo::MODE::WORLD;
            }
            ImGui::SameLine();

            if (ImGui::RadioButton("Local", editorContext_->currentMode_ == ImGuizmo::MODE::LOCAL))
            {
                editorContext_->currentMode_ = ImGuizmo::MODE::LOCAL;
            }

            ImGui::SameLine();
            WorldState current_state = World::GetInstance().GetState();
            bool isPlay = current_state == WorldState::Play;
            if (ImGui::Checkbox("Play", &isPlay))
            {
                if (current_state == WorldState::Play)
                {
                    World::GetInstance().SetState(WorldState::Edit);
                }
                else if (current_state == WorldState::Edit)
                {
                    World::GetInstance().SetState(WorldState::Play);
                }
            }

            ImGui::EndChild();
        }

        void PostViewportDrawWidgets()
        {
            DrawToolbar();

            auto current_size = ImGui::GetWindowSize();

            const float header_h = ImGui::GetTextLineHeightWithSpacing();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + header_h, current_size.x, current_size.y);

            auto l_camera = camera_.lock();

            auto cmp = l_camera->GetComponent<CameraComponent>()->GetCamera();

            auto view = cmp.GetView();
            auto proj = cmp.GetProj();

            /*auto grid_transform = DirectX::SimpleMath::Matrix::Identity;

            ImGuizmo::DrawGrid((float*)view.m, (float*)proj.m, (float*)grid_transform.m, 100.0f);*/

            if (auto go = editorContext_->selectedGameObject.lock())
            {
                auto transform = go->GetTransformComponent().lock();
                auto transform_matrix = transform->GetTransform().GetMatrix();
                if (ImGuizmo::Manipulate((float*)view.m, (float*)proj.m, editorContext_->currentOperation_, editorContext_->currentMode_, (float*)transform_matrix.m))
                {
                    transform->SetTransform(Transform::TransformFromMatrix(transform_matrix));
                }
            }
        }


        void UpdateCameraInfo(RenderContext& context)
        {
            auto camera_go = camera_.lock();
            auto camera = camera_go->GetComponent<CameraComponent>()->GetCamera();

            viewProjectionMatrix = camera.GetViewProj();

            RenderPassViewMatricies const_buf_data = {
                camera.GetView().Transpose(), camera.GetProj().Transpose(),
                camera.GetView().Invert().Transpose(), camera.GetProj().Invert().Transpose(),
                camera_go->GetTransformComponent().lock()->GetWorldLocation()
            };

            const auto CameraMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&const_buf_data), sizeof(RenderPassViewMatricies)};

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, sizeof(RenderPassViewMatricies));

            viewInfoConstBuffer = context.AllocateDynamicConstantView(CameraMatricesSpan, desc);

            ScreenDimensions size_const_data = {viewport_size_};

            const auto ScreenDimensionsSpan = std::span{reinterpret_cast<uint8_t*>(&size_const_data), sizeof(ScreenDimensions)};

            desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, sizeof(ScreenDimensions));

            screenDimensionsConstBuffer = context.AllocateDynamicConstantView(ScreenDimensionsSpan, desc);
        }
    };
}
