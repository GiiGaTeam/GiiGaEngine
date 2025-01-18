#pragma once

#include<d3d12.h>
#include<directxtk12/DDSTextureLoader.h>
#include<directxtk12/ResourceUploadBatch.h>
#include<stdexcept>
#include<filesystem>
#include<memory>

#include<AssetLoader.h>
#include<AssetType.h>
#include<ConcreteAsset/TextureAsset.h>
#include<Engine.h>
#include<DirectXUtils.h>

namespace GiiGa
{
    class DDSAssetLoader : public AssetLoader
    {
    protected:

    public:
        virtual ~DDSAssetLoader() = default;

        DDSAssetLoader() {
            id_ = Uuid::FromString("6238aaca-cd5b-4b6b-9578-ad5412bd1f47").value();
            pattern_ = R"((.+)\.dds)";
            type_ = AssetType::Texture2D;
        }

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path) {
            return {
                std::make_pair(AssetHandle { Uuid::New(), 0}, AssetMeta{
                    type_,
                    relative_path,
                    id_,
                    relative_path.stem().string()
                    })
            };
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

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving DDS textures is not supported.");
        }

        const char* GetName() override {
            return "DDS Texture Loader";
        }
    };
}
