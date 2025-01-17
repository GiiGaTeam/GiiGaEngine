#pragma once
#include <pybind11/conduit/wrap_include_python_h.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<directxtk12/WICTextureLoader.h>
#include<directxtk12/ResourceUploadBatch.h>
#include<stdexcept>
#include<filesystem>
#include<memory>

#include<AssetLoader.h>
#include<AssetType.h>
#include<TextureAsset.h>
#include<DirectXUtils.h>
#include<Engine.h>

namespace GiiGa
{
    class ImageAssetLoader : public AssetLoader
    {
    protected:

    public:
        ImageAssetLoader() {
            id_ = Uuid::FromString("a2227622-70bb-4b66-bab0-1d92d292a75c").value();
            pattern_ = R"((.+)\.(png|jpg|jpeg))";
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

            HRESULT hr = DirectX::CreateWICTextureFromFile(
                device.get(),
                upload_batch,
                path.c_str(),
                &texture
            );

            if (FAILED(hr))
                throw std::runtime_error("Failed to load PNG/JPEG texture: " + path.string());

            auto future = upload_batch.End(rs->GetRenderContext().getGraphicsCommandQueue().get());
            future.wait();

            auto texture_ptr = std::shared_ptr<ID3D12Resource>(texture, DXDelayedDeleter{ rs->GetRenderDevice() });

            return std::make_shared<TextureAsset>(handle, rs->GetRenderDevice(), texture_ptr);
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving PNG/JPEG textures is not supported.");
        }

        const char* GetName() override {
            return "PNG/JPEG Texture Loader";
        }
    };
}
