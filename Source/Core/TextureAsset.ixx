module;

#include <vector>
#include <filesystem>

export module TextureAsset;

import AssetBase;
import GPULocalResource;

namespace GiiGa
{
    export class TextureAsset : public AssetBase, GPULocalResource
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

        EventDispatcher<AssetHandle> OnDestroy;
    };
}  // namespace GiiGa
