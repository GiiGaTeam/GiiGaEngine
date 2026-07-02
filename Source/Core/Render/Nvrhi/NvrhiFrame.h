#pragma once

// NvrhiFrame.h
//
// Per-frame context handed to each NVRHI render pass. Deliberately tiny: just the two
// things a pass needs to record commands. It replaces the legacy RenderContext, which
// bundled a D3D12 command list, descriptor heaps, fences, and deferred-upload plumbing
// (all of which NVRHI now manages internally).

#include "NvrhiCommon.h"

namespace GiiGa::rhi
{
    struct FrameContext
    {
        nvrhi::IDevice* device = nullptr;
        nvrhi::ICommandList* cmd = nullptr;
    };

    // Components/pass-users implement this to push per-frame data to the GPU.
    // Replaces the legacy IUpdateGPUData (which took a RenderContext&).
    struct IFrameUpdater
    {
        virtual ~IFrameUpdater() = default;
        virtual void updateGPUData(nvrhi::IDevice* device, nvrhi::ICommandList* cmd) = 0;
    };
}
