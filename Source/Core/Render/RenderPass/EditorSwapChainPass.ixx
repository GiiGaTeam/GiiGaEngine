module;

#include <d3d12.h>
#include <vector>
#include <memory>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl2.h>

export module EditorSwapChainPass;

import RenderSystemSettings;
import RenderDevice;
import RenderPass;
import SwapChain;
import EditorViewport;
import DescriptorHeap;
import Window;

namespace GiiGa
{
    export class EditorSwapChainPass final : public RenderPass
    {
        friend class EditorRenderSystem;
    public:
        EditorSwapChainPass(RenderDevice& device, std::shared_ptr<SwapChain> swapChain):
            swapChain_(swapChain)
        {
            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            // Enable Gamepad Controls

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            GPUDescriptorHeap& descriptor_heap = device.GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            imgui_srv_desc_heap_allocation_ = descriptor_heap.Allocate(1);

            // Setup Platform/Renderer backends
            ImGui_ImplDX12_Init(device.GetDevice().get(), RenderSystemSettings::NUM_FRAMES_IN_FLIGHT,
                                DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap.GetDescriptorHeap().get(),
                                imgui_srv_desc_heap_allocation_.GetCpuHandle(),
                                imgui_srv_desc_heap_allocation_.GetGpuHandle());
        }

        void Draw(RenderContext& context) override
        {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            for (auto viewport : viewports_)
            {
                viewport->Execute(context);
            }
            // todo: ImGui Call
            // Start the Dear ImGui frame

            bool show_demo_window = true;
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            ImGui::ShowDemoWindow(&show_demo_window);

            ImGui::Render();

            swapChain_->Reset(context);

            // Render Dear ImGui graphics
            const float clear_color_with_alpha[4] = {0, 0, 0, 1};

            context.ClearRenderTargetView(swapChain_->getRTVDescriptorHandle(), clear_color_with_alpha);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain_->getRTVDescriptorHandle();

            //todo: add OMSetRenderTargets
            context.graphics_command_list_->OMSetRenderTargets(
                1, &rtv, FALSE, nullptr);


            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.graphics_command_list_.get());

            swapChain_->TransitToPresent(context);
        }

    private:
        std::shared_ptr<SwapChain> swapChain_;
        std::vector<std::shared_ptr<Viewport>> viewports_;
        DescriptorHeapAllocation imgui_srv_desc_heap_allocation_;
    };
}
