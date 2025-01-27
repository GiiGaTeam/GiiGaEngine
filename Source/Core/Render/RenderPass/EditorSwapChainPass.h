#pragma once

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl2.h>
#include <ImGuizmo.h>


#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<vector>
#include<memory>

#include<RenderSystemSettings.h>
#include<RenderDevice.h>
#include<RenderPass.h>
#include<SwapChain.h>
#include<EditorViewport.h>
#include<DescriptorHeap.h>
#include<EditorContext.h>
#include<IImGuiWindow.h>
#include<ImGuiSceneHierarchy.h>
#include<ImGuiInspector.h>
#include<ImGuiAssetEditor.h>
#include<ImGuiContentBrowser.h>
#include<SceneVisibility.h>

namespace GiiGa
{
    class EditorSwapChainPass final : public RenderPass
    {
        friend class EditorRenderSystem;

    public:
        EditorSwapChainPass(RenderDevice& device, std::shared_ptr<SwapChain> swapChain):
            swapChain_(swapChain),
            editorContext_(std::make_shared<EditorContext>())
        {
            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            GPUDescriptorHeap& descriptor_heap = device.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            imgui_srv_desc_heap_allocation_ = descriptor_heap.Allocate(1);

            // Setup Platform/Renderer backends
            ImGui_ImplDX12_Init(device.GetDxDevice().get(), RenderSystemSettings::NUM_FRAMES_IN_FLIGHT,
                                DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap.GetDescriptorHeap().get(),
                                imgui_srv_desc_heap_allocation_.GetCpuHandle(),
                                imgui_srv_desc_heap_allocation_.GetGpuHandle());

            windows_.push_back(std::make_unique<ImGuiSceneHierarchy>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiInspector>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiAssetEditor>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiContentBrowser>(editorContext_));
        }

        ~EditorSwapChainPass() override
        {
            ImGui_ImplDX12_Shutdown();
        }

        void Draw(RenderContext& context) override
        {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            BeginDockSpace_();
            {
                for (auto&& window : windows_)
                {
                    window->RecordImGui();
                }

                // Tick scene visibility in case it was changed in imgui
                SceneVisibility::Tick();

                //ImGui::ShowDemoWindow();

                for (auto viewport : viewports_)
                {
                    viewport->Execute(context);
                }
            }
            EndDockSpace_();

            swapChain_->Reset(context);
            
            // Render Dear ImGui graphics
            context.ClearRenderTargetView(swapChain_->getRTVDescriptorHandle(), {.Color = {0, 0, 0, 1}});

            D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain_->getRTVDescriptorHandle();

            //todo: add OMSetRenderTargets
            context.graphics_command_list_->OMSetRenderTargets(
                1, &rtv, FALSE, nullptr);


            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.graphics_command_list_.get());

            swapChain_->TransitToPresent(context);
        }

    private:
        std::shared_ptr<SwapChain> swapChain_;
        std::shared_ptr<EditorContext> editorContext_;
        std::vector<std::shared_ptr<Viewport>> viewports_;
        std::vector<std::unique_ptr<IImGuiWindow>> windows_;
        DescriptorHeapAllocation imgui_srv_desc_heap_allocation_;

        void BeginDockSpace_()
        {
            bool p_open = true;
            static bool opt_fullscreen = true;
            static bool opt_padding = false;
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
            // because it would be confusing to have two docking targets within each others.ImGuiWindowFlags_MenuBar
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
            if (opt_fullscreen)
            {
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            }
            else
            {
                dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
            }

            // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
            // and handle the pass-thru hole, so we ask Begin() to not render a background.
            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
            // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
            // all active windows docked into it will lose their parent and become undocked.
            // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
            // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
            if (!opt_padding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace Demo", &p_open, window_flags);
            if (!opt_padding)
                ImGui::PopStyleVar();

            if (opt_fullscreen)
                ImGui::PopStyleVar(2);

            // Submit the DockSpace
            const ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                const ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }
            else
            {
                throw std::exception("Turn on docking");
            }
        }

        void EndDockSpace_()
        {
            ImGui::End();
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::Render();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }
    };
}
