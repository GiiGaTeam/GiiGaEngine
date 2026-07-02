#pragma once

// NvrhiRenderPass.h
//
// Minimal render-pass abstraction for the NVRHI path: a pass is anything with an
// execute(FrameContext) called once per frame, in order. A small ordered list of passes
// (RenderGraph) is the frame's pipeline. This mirrors the legacy RenderPass/RenderGraph
// shape but operates on nvrhi handles instead of a D3D12 command list + descriptor heaps.

#include "NvrhiCommon.h"
#include "NvrhiFrame.h"

#include <functional>
#include <memory>
#include <vector>

namespace GiiGa::rhi
{
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;
        virtual void execute(const FrameContext& ctx) = 0;
    };

    using RenderPassHandle = std::shared_ptr<RenderPass>;

    // Ordered list of passes executed sequentially each frame.
    class RenderGraph
    {
    public:
        void addPass(RenderPassHandle pass) { passes_.push_back(std::move(pass)); }
        void execute(const FrameContext& ctx)
        {
            for (auto& p : passes_) p->execute(ctx);
        }
        void clear() { passes_.clear(); }
    private:
        std::vector<RenderPassHandle> passes_;
    };
}
