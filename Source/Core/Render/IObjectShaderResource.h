#pragma once

#include <nvrhi/nvrhi.h>

namespace GiiGa
{
    // Per-object/per-material shader resources. Instead of returning raw GPU descriptor
    // handles (D3D12), an implementation now appends its bindings (e.g. the material
    // constant buffer at b2 and textures t0..t7) to the binding set the render pass is
    // assembling for a draw.
    struct IObjectShaderResource
    {
        virtual ~IObjectShaderResource() = default;
        virtual void AddBindings(nvrhi::BindingSetDesc& set) = 0;
    };
}
