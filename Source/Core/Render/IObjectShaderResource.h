#pragma once

#include<vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>

namespace GiiGa
{
    // Per-object/per-material shader bindings. Implementations return
    // binding-set items with the engine's canonical register slots:
    //   materials: constant buffer -> b2, textures -> t(TexturesOrder - 1)
    //   lights:    constant buffer -> b3
    // Render passes filter these items against each pipeline's binding layout.
    struct IObjectShaderResource
    {
        virtual ~IObjectShaderResource() = default;
        virtual std::vector<nvrhi::BindingSetItem> GetBindingItems() =0;
    };
}
