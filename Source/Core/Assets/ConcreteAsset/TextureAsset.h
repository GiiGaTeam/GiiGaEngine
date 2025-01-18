#pragma once


#include<AssetBase.h>
#include<GPULocalResource.h>

namespace GiiGa
{
    class TextureAsset : public AssetBase, public GPULocalResource
    {
    public:
        TextureAsset(AssetHandle handle, RenderDevice& device, std::shared_ptr<ID3D12Resource> resource)
            : GPULocalResource(device, resource)
            , AssetBase(handle)
        {
        }

        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;

        virtual AssetType GetType() override {
            return AssetType::Texture2D;
        }
    };
}  // namespace GiiGa
