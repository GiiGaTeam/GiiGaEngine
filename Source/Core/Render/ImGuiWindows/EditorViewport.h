#pragma once

#include <pybind11/embed.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directxtk12/SimpleMath.h>
#include <ImGuizmo.h>

#include <imgui.h>
#include <string>
#include <memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>

#include <Viewport.h>
#include <RenderDevice.h>
#include <DescriptorHeap.h>
#include <Core/Render/Nvrhi/NvrhiCommon.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiRenderPass.h>
#include <Core/Render/Nvrhi/NvrhiShader.h>
#include <RenderPassViewData.h>
#include <GBufferPass.h>
#include <GLightPass.h>
#include <ShadowPass.h>
#include <PostProcessPass.h>
#include <DebugPass.h>
#include <ForwardPass.h>
#include "World.h"
#include <CameraComponent.h>
#include <GameObject.h>
#include <SpectatorMovementComponent.h>
#include <EditorContext.h>
#include <Engine.h>

namespace GiiGa
{
    class EditorViewport : public Viewport, public std::enable_shared_from_this<EditorViewport>
    {
    public:
        EditorViewport(RenderDevice& device, std::shared_ptr<EditorContext> editor_ctx)
            : Viewport(device)
            , editorContext_(editor_ctx)
        {
            if (auto rs = Engine::Instance().RenderSystem())
                nvrhiDevice_ = rs->GetNvrhiDevice();
        }

        void Resize(DirectX::SimpleMath::Vector2 new_size) override
        {
            Viewport::Resize(new_size);

            if (auto l_camera = camera_.lock())
            {
                if (auto cam = l_camera->GetComponent<CameraComponent>())
                    cam->SetAspect(new_size.x / new_size.y);
            }
        }

        void Init(RenderContext& context) override
        {
            Resize(viewport_size_);

            renderGraph_ = std::make_unique<rhi::RenderGraph>();
            shaderCache_ = std::make_unique<rhi::ShaderCache>();

            auto camFn = [this]() { return this->GetCameraInfo(); };
            auto resFn = [this]() { return this->GetResultInfo(); };

            renderGraph_->addPass(std::make_shared<ShadowPass>(nvrhiDevice_, *shaderCache_, camFn));
            renderGraph_->addPass(std::make_shared<GBufferPass>(nvrhiDevice_, *shaderCache_, camFn, gbuffer_));
            renderGraph_->addPass(std::make_shared<GLightPass>(nvrhiDevice_, *shaderCache_, camFn, gbuffer_));
            renderGraph_->addPass(std::make_shared<ForwardPass>(nvrhiDevice_, *shaderCache_, camFn, gbuffer_));
            renderGraph_->addPass(std::make_shared<DebugPass>(nvrhiDevice_, *shaderCache_, camFn, gbuffer_));
            renderGraph_->addPass(std::make_shared<PostProcessPass>(nvrhiDevice_, *shaderCache_, resFn, gbuffer_));

            if (nvrhiDevice_ && device_.GetDxDevice())
            {
                resultSrvAllocation_ = device_.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Allocate(1);
            }

            camera_ = GameObject::CreateEmptyGameObject({.name = "Viewport Camera"});
            const auto cameraComponent = camera_.lock()->CreateComponent<CameraComponent>(Perspective, 90, 16 / 9);
            camera_.lock()->CreateComponent<SpectatorMovementComponent>();
            std::dynamic_pointer_cast<GameObject>(cameraComponent->GetOwner())->GetTransformComponent().lock()->SetLocation({-5, 3, 4});
            std::dynamic_pointer_cast<GameObject>(cameraComponent->GetOwner())->GetTransformComponent().lock()->SetRotation({-21, -40, 0});
        }

        void SetCamera(std::shared_ptr<CameraComponent> camera) override
        {
            camera_ = std::dynamic_pointer_cast<GameObject>(camera->GetOwner());
            camera->SetAspect(viewport_size_.x / viewport_size_.y);
        }

        RenderPassViewData GetCameraInfo() override
        {
            RenderPassViewData data;
            if (!camera_.expired())
            {
                data.camera = camera_.lock()->GetComponent<CameraComponent>()->GetCamera();
            }
            data.viewConstants = viewConstants_;
            data.screenDimensions = screenDimensions_;
            return data;
        }

        void Execute(RenderContext& context) override
        {
            if (camera_.expired()) return;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

            if (ImGui::Begin(("Viewport" + std::to_string(viewport_index)).c_str(), nullptr, ImGuiWindowFlags_NoScrollWithMouse))
            {
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                {
                    if (const auto movement = camera_.lock()->GetComponent<SpectatorMovementComponent>())
                    {
                        auto pos = ImGui::GetCursorScreenPos();
                        pos.x += ImGui::GetContentRegionAvail().x * 0.5;
                        pos.y += ImGui::GetContentRegionAvail().y * 0.5;
                        Engine::Instance().Window()->SetMouseLock(ImGui::IsMouseDown(ImGuiMouseButton_Right), pos.x, pos.y);
                        movement->active_ = ImGui::IsMouseDown(ImGuiMouseButton_Right);
                    }
                }
            }

            UpdateCameraInfo(context);

            auto current_size = ImGui::GetContentRegionAvail();
            if (current_size.y < 0)
            {
                current_size.y = 2;
                ImGui::Text("Content height is negative!");
            }

            if (current_size.x != viewport_size_.x || current_size.y != viewport_size_.y)
                Resize({current_size.x, current_size.y});

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::AllowAxisFlip(false);
            ImGuizmo::SetDrawlist();

            // --- run the NVRHI render graph on the engine's NVRHI command list ---
            // TODO(runtime): the NVRHI command list and the legacy RenderContext command list
            // share one D3D12 queue; submitting the NVRHI list mid-frame here is not synchronized
            // with the legacy ImGui command list. Consolidating onto a single command list (the
            // rhi::RenderSystem frame loop) is the intended end state.
            if (nvrhiDevice_ && renderGraph_)
            {
                nvrhi::ICommandList* cmd = Engine::Instance().RenderSystem()->GetUploadCommandList();
                if (cmd)
                {
                    cmd->open();
                    rhi::FrameContext fc{nvrhiDevice_, cmd};
                    renderGraph_->execute(fc);
                    cmd->close();
                    nvrhiDevice_->executeCommandList(cmd);
                }
            }

            RefreshResultSrv_();

            if (resultSrvResource_)
            {
                ImGui::Image(static_cast<ImTextureID>(resultSrvAllocation_.GetGpuHandle().ptr),
                             ImVec2(viewport_size_.x, viewport_size_.y));
            }

            PostViewportDrawWidgets();

            ImGui::End();
            ImGui::PopStyleVar();
        }

    private:
        int viewport_index = 0;
        std::weak_ptr<GameObject> camera_;
        DirectX::SimpleMath::Matrix viewProjectionMatrix;

        rhi::ViewConstants viewConstants_;
        rhi::ScreenDimensions screenDimensions_;

        std::shared_ptr<EditorContext> editorContext_;
        std::unique_ptr<rhi::ShaderCache> shaderCache_;
        std::unique_ptr<rhi::RenderGraph> renderGraph_;

        DescriptorHeapAllocation resultSrvAllocation_;
        ID3D12Resource* resultSrvResource_ = nullptr;

        void RefreshResultSrv_()
        {
            if (!nvrhiDevice_ || !rtvData_.resource) return;
            ID3D12Resource* native = rtvData_.resource->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
            if (resultSrvResource_ == native) return;
            resultSrvResource_ = native;
            if (!resultSrvAllocation_.GetCpuHandle().ptr) return;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            device_.GetDxDevice()->CreateShaderResourceView(native, &srvDesc, resultSrvAllocation_.GetCpuHandle());
        }

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

        void UpdateCameraInfo(RenderContext& /*context*/)
        {
            auto camera_go = camera_.lock();
            if (!camera_go) return;
            auto camComp = camera_go->GetComponent<CameraComponent>();
            if (!camComp) return;
            auto camera = camComp->GetCamera();

            viewProjectionMatrix = camera.GetViewProj();

            viewConstants_.viewMatrix = camera.GetView().Transpose();
            viewConstants_.projMatrix = camera.GetProj().Transpose();
            viewConstants_.invViewMatrix = camera.GetView().Invert().Transpose();
            viewConstants_.invProjMatrix = camera.GetProj().Invert().Transpose();

            auto t = camera_go->GetTransformComponent().lock();
            viewConstants_.camPos = t ? t->GetWorldLocation() : DirectX::SimpleMath::Vector3();

            screenDimensions_.dimensions = viewport_size_;
        }
    };
}
