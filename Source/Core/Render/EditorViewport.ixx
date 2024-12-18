module;

#include <imgui.h>
#include <string>
#include <memory>
#include <d3d12.h>
#include <iostream>

export module EditorViewport;

export import Viewport;
import RenderDevice;
import ForwardPass;
import RenderGraph;
import CameraComponent;

namespace GiiGa
{
    export class EditorViewport : public Viewport, public std::enable_shared_from_this<EditorViewport>
    {
    public:
        EditorViewport(RenderDevice& device, std::shared_ptr<CameraComponent> camera = nullptr):
            Viewport(device, camera)
        {
            Resize(viewport_size_);
        }

        void Init() override
        {
            renderGraph_ = std::make_shared<RenderGraph>();
            renderGraph_->AddPass(std::make_shared<ForwardPass>());
        }

        DescriptorHeapAllocation GetCameraDescriptor() override
        {
            return {};
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

            context.ClearRenderTargetView(resultRTV_->getDescriptor().GetCpuHandle(), clear_color_with_alpha);

            // draw here
            renderGraph_->Draw(context, shared_from_this());
            // draw here - end

            resultResource_->StateTransition(context, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            ImGui::Image((ImTextureID)resultSRV_->getDescriptor().getGPUHandle().ptr, ImVec2(viewport_size_.x, viewport_size_.y));

            ImGui::End();
        }

    private:
        int viewport_index = 0;
    };
}
