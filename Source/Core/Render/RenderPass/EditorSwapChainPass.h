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
#include<tracy/tracy/Tracy.hpp>
#include<TracyGpu.h>

#include<RenderSystemSettings.h>
#include<RenderDevice.h>
#include<RenderPass.h>
#include<SwapChain.h>
#include<EditorViewport.h>
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
            device_(device),
            swapChain_(swapChain),
            editorContext_(std::make_shared<EditorContext>())
        {
            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            // Setup Platform/Renderer backends. SRV descriptors for ImGui (fonts,
            // icons, viewport textures) come from the dedicated heap in RenderDevice.
            ImGui_ImplDX12_InitInfo init_info;
            init_info.Device = device.GetDxDevice().get();
            init_info.CommandQueue = device.GetGraphicsQueue().get();
            init_info.NumFramesInFlight = RenderSystemSettings::NUM_FRAMES_IN_FLIGHT;
            init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            init_info.UserData = &device;
            init_info.SrvDescriptorHeap = device.GetImGuiSrvHeap();
            init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
            {
                auto handle = static_cast<RenderDevice*>(info->UserData)->AllocateImGuiDescriptor();
                *out_cpu = handle.cpu;
                *out_gpu = handle.gpu;
            };
            init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
            {
                auto device = static_cast<RenderDevice*>(info->UserData);
                auto heap_start = device->GetImGuiSrvHeap()->GetGPUDescriptorHandleForHeapStart();
                auto increment = device->GetDxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                device->FreeImGuiDescriptor(static_cast<uint32_t>((gpu.ptr - heap_start.ptr) / increment));
            };

            ImGui_ImplDX12_Init(&init_info);

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
            ZoneScoped;
            GIIGA_GPU_ZONE(context, "EditorSwapChain");
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            BeginDockSpace_();
            {
                {
                    ZoneScopedN("RecordImGui");
                    for (auto&& window : windows_)
                    {
                        window->RecordImGui();
                    }
                }

                // Tick scene visibility in case it was changed in imgui
                SceneVisibility::Tick();

                for (auto viewport : viewports_)
                {
                    viewport->Execute(context);
                }
            }
            EndDockSpace_();

            // ImGui renders through native D3D12 on the same command list, after
            // all NVRHI work of this frame has been recorded.
            ID3D12GraphicsCommandList* native_cmd_list = context.GetNativeCommandList();

            swapChain_->BeginRender(native_cmd_list);

            ID3D12DescriptorHeap* heaps[] = {device_.GetImGuiSrvHeap()};
            native_cmd_list->SetDescriptorHeaps(1, heaps);

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), native_cmd_list);

            swapChain_->EndRender(native_cmd_list);

            // The raw commands above invalidated NVRHI's cached command list state.
            context.CommandList()->clearState();
        }

    private:
        RenderDevice& device_;
        std::shared_ptr<SwapChain> swapChain_;
        std::shared_ptr<EditorContext> editorContext_;
        std::vector<std::shared_ptr<Viewport>> viewports_;
        std::vector<std::unique_ptr<IImGuiWindow>> windows_;

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
