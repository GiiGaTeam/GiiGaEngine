#pragma once

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl2.h>
#include <ImGuizmo.h>


#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<wrl/client.h>
#include<vector>
#include<memory>

#include<RenderSystemSettings.h>
#include<RenderDevice.h>
#include<EditorViewport.h>
#include<DescriptorHeap.h>
#include<EditorContext.h>
#include<IImGuiWindow.h>
#include<ImGuiSceneHierarchy.h>
#include<ImGuiInspector.h>
#include<ImGuiAssetEditor.h>
#include<ImGuiContentBrowser.h>
#include<SceneVisibility.h>
#include <Core/Render/Nvrhi/NvrhiCommon.h>
#include <Core/Render/Nvrhi/NvrhiSwapChain.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiRenderPass.h>

namespace GiiGa
{
    // Editor root pass: owns the dockspace + all ImGui windows, executes viewports
    // (which run the deferred render graph), then composites ImGui draw data onto
    // the swap-chain back buffer via the imgui_impl_dx12 backend on the native D3D12
    // command list extracted from the NVRHI command list.
    class EditorSwapChainPass final : public rhi::RenderPass
    {
        friend class EditorRenderSystem;

    public:
        EditorSwapChainPass(RenderDevice& device, rhi::SwapChain* swapChain):
            rhiSwapChain_(swapChain),
            editorContext_(std::make_shared<EditorContext>())
        {
            ImGui::StyleColorsDark();

            GPUDescriptorHeap& descriptor_heap = device.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            imgui_srv_desc_heap_allocation_ = descriptor_heap.Allocate(1);

            ID3D12Device* d3dDevice = device.GetDxDevice().get();

            ImGui_ImplDX12_Init(d3dDevice, RenderSystemSettings::NUM_FRAMES_IN_FLIGHT,
                                DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap.GetDescriptorHeap().get(),
                                imgui_srv_desc_heap_allocation_.GetCpuHandle(),
                                imgui_srv_desc_heap_allocation_.GetGpuHandle());

            // RTV descriptor heap for swap-chain back buffers (needed by imgui_impl_dx12).
            uint32_t bufferCount = swapChain->getBufferCount();
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.NumDescriptors = bufferCount;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
            rtvDescriptorSize_ = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            windows_.push_back(std::make_unique<ImGuiSceneHierarchy>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiInspector>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiAssetEditor>(editorContext_));
            windows_.push_back(std::make_unique<ImGuiContentBrowser>(editorContext_));
        }

        ~EditorSwapChainPass() override
        {
            ImGui_ImplDX12_Shutdown();
        }

        void execute(const rhi::FrameContext& ctx) override
        {
            // --- ImGui UI build ---
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

                SceneVisibility::Tick();

                for (auto viewport : viewports_)
                {
                    viewport->Execute(ctx);
                }
            }
            EndDockSpace_();

            // --- Composite ImGui onto the swap-chain back buffer ---
            nvrhi::ITexture* backBuffer = rhiSwapChain_->getCurrentTexture();
            if (!backBuffer) return;

            // NVRHI clears the back buffer (Present -> RenderTarget automatically).
            ctx.cmd->clearTextureFloat(backBuffer, nvrhi::TextureSubresourceSet(), {0, 0, 0, 1});

            // Extract the native D3D12 command list for ImGui rendering.
            auto* nativeCmd = static_cast<ID3D12GraphicsCommandList*>(
                ctx.cmd->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList));
            auto* d3dDevice = static_cast<ID3D12Device*>(
                ctx.device->getNativeObject(nvrhi::ObjectTypes::D3D12_Device));

            // Ensure an RTV exists for the current back buffer (re-created on resize).
            ID3D12Resource* nativeResource = static_cast<ID3D12Resource*>(
                backBuffer->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource));
            uint32_t idx = rhiSwapChain_->getCurrentIndex();
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            rtv.ptr += static_cast<SIZE_T>(idx * rtvDescriptorSize_);
            d3dDevice->CreateRenderTargetView(nativeResource, nullptr, rtv);

            nativeCmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), nativeCmd);

            // NVRHI close() will transition the back buffer RenderTarget -> Present.
        }

    private:
        rhi::SwapChain* rhiSwapChain_;
        std::shared_ptr<EditorContext> editorContext_;
        std::vector<std::shared_ptr<Viewport>> viewports_;
        std::vector<std::unique_ptr<IImGuiWindow>> windows_;
        DescriptorHeapAllocation imgui_srv_desc_heap_allocation_;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
        UINT rtvDescriptorSize_ = 0;

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
