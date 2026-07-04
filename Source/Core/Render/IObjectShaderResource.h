#pragma once

// IObjectShaderResource.h — per-object/per-material shader resource provider.
// NVRHI cutover: instead of returning a flat vector of D3D12_GPU_DESCRIPTOR_HANDLE, the
// resource exposes the actual nvrhi resources (material constant buffer + textures) so the
// pass can build a nvrhi::BindingSet for them. Index into `textures` follows Material's
// TexturesOrder (BaseColor, Metallic, Specular, Roughness, Anisotropy, EmissiveColor, ...).

#include <array>
#include <nvrhi/nvrhi.h>

namespace GiiGa
{
    inline constexpr size_t MaxMaterialTextures = 8;

    struct MaterialBindings
    {
        nvrhi::IBuffer* constantBuffer = nullptr;                         // material CB (b2); may be null
        std::array<nvrhi::ITexture*, MaxMaterialTextures> textures{};     // by semantic; nullptr if absent
        bool opaque = true;
    };

    struct IObjectShaderResource
    {
        virtual ~IObjectShaderResource() = default;
        virtual const MaterialBindings& GetMaterialBindings() = 0;
    };
}
