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

#include<nvrhi/nvrhi.h>
#include<nvrhi/d3d12.h>

#include<RenderSystemSettings.h>
#include<RenderDevice.h>
#include<RenderContext.h>
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
            swapChain_(swapChain),
            editorContext_(std::make_shared<EditorContext>())
        {
            ImGui::StyleColorsDark();

            // Share NVRHI's shader-visible SRV heap with the imgui DX12 backend: allocate one
            // descriptor (for the font atlas) from it and pass the heap + handles to imgui.
            auto* d12dev = device.GetD3D12NvrhiDevice();
            srv_heap_ = d12dev->getDescriptorHeap(nvrhi::d3d12::DescriptorHeapType::ShaderResourceView);
            imgui_srv_index_ = srv_heap_->allocateDescriptor();

            ImGui_ImplDX12_Init(device.GetD3D12Device(), RenderSystemSettings::NUM_FRAMES_IN_FLIGHT,
                                RenderSystemSettings::BACK_BUFFER_FORMAT, srv_heap_->getShaderVisibleHeap(),
                                srv_heap_->getCpuHandleShaderVisible(imgui_srv_index_),
                                srv_heap_->getGpuHandle(imgui_srv_index_));

            windows_.push_back(std::make_unique<ImGuiSceneHierarchy>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiInspector>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiAssetEditor>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiContentBrowser>(editorContext_));
        }

        ~EditorSwapChainPass() override
        {
            ImGui_ImplDX12_Shutdown();
            if (srv_heap_)
                srv_heap_->releaseDescriptor(imgui_srv_index_);
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
                    window->RecordImGui();

                // Tick scene visibility in case it was changed in imgui
                SceneVisibility::Tick();

                for (auto viewport : viewports_)
                    viewport->Execute(context);
            }
            EndDockSpace_();

            auto* cl = context.GetCommandList();
            nvrhi::ITexture* backBuffer = swapChain_->GetCurrentBackBufferTexture();

            cl->setTextureState(backBuffer, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
            cl->commitBarriers();
            cl->clearTextureFloat(backBuffer, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

            // imgui records native D3D12 into NVRHI's command list; it's the last thing this frame.
            auto* nativeCL = static_cast<ID3D12GraphicsCommandList*>(
                cl->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList).pointer);
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain_->GetCurrentBackBufferRTV();
            nativeCL->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), nativeCL);

            cl->setTextureState(backBuffer, nvrhi::AllSubresources, nvrhi::ResourceStates::Present);
            cl->commitBarriers();
        }

    private:
        std::shared_ptr<SwapChain> swapChain_;
        std::shared_ptr<EditorContext> editorContext_;
        std::vector<std::shared_ptr<Viewport>> viewports_;
        std::vector<std::unique_ptr<IImGuiWindow>> windows_;
        nvrhi::d3d12::IDescriptorHeap* srv_heap_ = nullptr;
        nvrhi::d3d12::DescriptorIndex imgui_srv_index_ = 0;

        void BeginDockSpace_()
        {
            bool p_open = true;
            static bool opt_fullscreen = true;
            static bool opt_padding = false;
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

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

            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            if (!opt_padding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace Demo", &p_open, window_flags);
            if (!opt_padding)
                ImGui::PopStyleVar();

            if (opt_fullscreen)
                ImGui::PopStyleVar(2);

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
