#pragma once

#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<nvrhi/nvrhi.h>

#include<AssetBase.h>
#include<NvrhiUtils.h>
#include<RenderDevice.h>

namespace GiiGa
{
    // GPU-resident texture: an nvrhi texture handle created around a native
    // resource loaded by DirectXTK12 (DDS/WIC).
    class TextureAsset : public AssetBase
    {
    public:
        TextureAsset(AssetHandle handle, RenderDevice& device, std::shared_ptr<ID3D12Resource> resource)
            : AssetBase(handle)
            , native_resource_(resource)
            , texture_(WrapNativeTexture(device.NvDevice(), resource.get(), "TextureAsset"))
        {
        }

        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;

        virtual AssetType GetType() override
        {
            return AssetType::Texture2D;
        }

        nvrhi::TextureHandle GetTexture() const
        {
            return texture_;
        }

    private:
        // Keeps the native resource alive alongside the nvrhi wrapper.
        std::shared_ptr<ID3D12Resource> native_resource_;
        nvrhi::TextureHandle texture_;
    };
} // namespace GiiGa
