#pragma once

#include<tracy/tracy/Tracy.hpp>
#include<tracy/tracy/TracyD3D12.hpp>

#include<RenderContext.h>

// Opens a scoped Tracy GPU zone on the render context's native D3D12 command
// list. The zone is recorded on the graphics queue owned by RenderDevice.
// Expands to nothing unless TRACY_ENABLE is defined.
#define GIIGA_GPU_ZONE(context, name) \
    TracyD3D12Zone((context).GetDevice().GetTracyGpuContext(), (context).GetNativeCommandList(), name)
