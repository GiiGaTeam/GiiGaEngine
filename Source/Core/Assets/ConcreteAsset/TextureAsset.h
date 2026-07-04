#pragma once

// TextureAsset.h — NVRHI-backed texture asset.
// The DDS/WIC loaders create the underlying ID3D12Resource with DirectXTK12 (robust decoding),
// then this asset wraps it as an NVRHI texture via createHandleForNativeTexture and keeps the
// native resource alive (NVRHI wraps non-owningly). Replaces the old GPULocalResource base.

#include <AssetBase.h>
#include <Core/Render/Nvrhi/NvrhiCommon.h>

#include <wrl/client.h>
#include <d3d12.h>

namespace GiiGa
{
    class TextureAsset : public AssetBase
    {
    public:
        // Takes ownership of `nativeResource` ( Attach, no AddRef ).
        TextureAsset(AssetHandle handle, nvrhi::IDevice* device, ID3D12Resource* nativeResource)
            : AssetBase(handle)
        {
            nativeResource_.Attach(nativeResource);

            D3D12_RESOURCE_DESC rd = nativeResource_->GetDesc();
            nvrhi::TextureDesc td;
            td.setWidth(static_cast<uint32_t>(rd.Width))
              .setHeight(rd.Height)
              .setMipLevels(rd.MipLevels)
              .setFormat(rhi::dxgiToNvrhi(rd.Format))
              .setInitialState(nvrhi::ResourceStates::ShaderResource)
              .setKeepInitialState(true)
              .setDebugName("texture asset");

            texture_ = device->createHandleForNativeTexture(
                nvrhi::ObjectTypes::D3D12_Resource,
                nvrhi::Object(static_cast<void*>(nativeResource_.Get())),
                td);
        }

        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;

        nvrhi::TextureHandle GetTexture() const { return texture_; }
        nvrhi::ITexture* Get() const { return texture_.Get(); }

        virtual AssetType GetType() override { return AssetType::Texture2D; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> nativeResource_;
        nvrhi::TextureHandle texture_;
    };
}
