module;

#include <imgui.h>
#include <string>
#include <memory>
#include <d3d12.h>

export module EditorViewport;

export import Viewport;
import RenderDevice;

namespace GiiGa
{
    export class EditorViewport : public Viewport
    {
    public:
        EditorViewport(RenderDevice& device):
            Viewport(device)
        {
            Resize(viewport_size_);
        }

        DescriptorHeapAllocation getCameraDescriptor() override
        {
            return {};
        }

        void Execute(RenderContext& context) override
        {
            ImGui::Begin(("Viewport" + std::to_string(viewport_index)).c_str());
            
            auto current_size = ImGui::GetWindowSize();

            if (current_size.x != viewport_size_.x || current_size.y != viewport_size_.y)
                Resize({current_size.x, current_size.y});

            resultResource_->StateTransition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);

            const float clear_color_with_alpha[4] = {1, 0, 0, 1};
            
            context.ClearRenderTargetView(resultRTV_->getDescriptor().GetCpuHandle(),clear_color_with_alpha);
            
            // draw here
            
            resultResource_->StateTransition(context, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            ImGui::Image((ImTextureID) resultSRV_->getDescriptor().getGPUHandle().ptr, ImVec2(viewport_size_.x, viewport_size_.y));

            ImGui::End();
        }

    private:
        int viewport_index = 0;
    };
}
