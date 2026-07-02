#pragma once

// NvrhiClearPass.h
//
// First NVRHI render pass: clears the swap-chain's current back buffer to a solid color.
// Trivial, but it exercises the whole foundation end-to-end (device -> command list ->
// wrapped back-buffer texture -> barrier -> clear -> present) and serves as the template
// for real passes (GBuffer, GLight, ...).

#include "NvrhiCommon.h"
#include "NvrhiRenderPass.h"
#include "NvrhiSwapChain.h"

namespace GiiGa::rhi
{
    class ClearPass : public RenderPass
    {
    public:
        explicit ClearPass(SwapChain* swapChain, nvrhi::Color color = {0.05f, 0.05f, 0.07f, 1.0f})
            : swapChain_(swapChain), color_(color) {}

        void execute(const FrameContext& ctx) override
        {
            if (!ctx.cmd || !swapChain_) return;
            if (nvrhi::ITexture* target = swapChain_->getCurrentTexture())
            {
                ctx.cmd->clearTextureFloat(target, nvrhi::AllSubresources, color_);
            }
        }

    private:
        SwapChain* swapChain_;
        nvrhi::Color color_;
    };
}
