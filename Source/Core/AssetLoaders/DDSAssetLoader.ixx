export module DDSAssetLoader;

import <d3d12.h>;
import <directxtk12/DDSTextureLoader.h>;
import <directxtk12/ResourceUploadBatch.h>;
import <stdexcept>;
import <filesystem>;
import <memory>;

import AssetLoader;
import AssetType;
import TextureAsset;
import Engine;
import DirectXUtils;

namespace GiiGa
{
    export class DDSAssetLoader : public AssetLoader
    {
    protected:

    public:
        virtual ~DDSAssetLoader() = default;

        DDSAssetLoader() {
            pattern_ = R"((.+)\.dds)";
            type_ = AssetType::Texture2D;
        }

        std::shared_ptr<AssetBase> Load(AssetHandle handle, const std::filesystem::path& path) override {
            if (!std::filesystem::exists(path))
                throw std::runtime_error("File does not exist: " + path.string());

            auto rs = Engine::Instance().RenderSystem();
            auto device = rs->GetRenderDevice().GetDxDevice();

            DirectX::ResourceUploadBatch upload_batch(device.get());
            upload_batch.Begin();

            ID3D12Resource* texture = nullptr;

            HRESULT hr = DirectX::CreateDDSTextureFromFile(
                device.get(),
                upload_batch,
                path.c_str(),
                &texture
            );

            if (FAILED(hr))
                throw std::runtime_error("Failed to load DDS texture: " + path.string());

            auto future = upload_batch.End(rs->GetRenderContext().getGraphicsCommandQueue().get());
            future.wait();

            auto texture_ptr = std::shared_ptr<ID3D12Resource>(texture, DXDelayedDeleter{ rs->GetRenderDevice() });

            return std::make_shared<TextureAsset>(handle, rs->GetRenderDevice(), texture_ptr);
        }

        void Save(AssetBase& asset, std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving DDS textures is not supported.");
        }

        const char* GetName() override {
            return "DDS Texture Loader";
        }
    };
}
