#pragma once

#include<nvrhi/nvrhi.h>

#include<AssetBase.h>

namespace GiiGa
{
    // GPU-resident texture asset. Holds an NVRHI texture (created by the DDS/Image loaders).
    class TextureAsset : public AssetBase
    {
    public:
        TextureAsset(AssetHandle handle, nvrhi::TextureHandle texture)
            : AssetBase(handle)
            , texture_(texture)
        {
        }

        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;

        nvrhi::ITexture* GetTexture() const { return texture_; }

        virtual AssetType GetType() override
        {
            return AssetType::Texture2D;
        }

    private:
        nvrhi::TextureHandle texture_;
    };
}  // namespace GiiGa
