module;

#include <d3d12.h>
#include <directxtk12/DDSTextureLoader.h>
#include "directxtk12/ResourceUploadBatch.h"
#include <stdexcept>
#include <filesystem>
#include <memory>

export module DDSAssetLoader;

import AssetLoader;
import AssetType;
import GPULocalResource;
import Engine;

namespace GiiGa
{
    export class DDSAssetLoader : public AssetLoader
    {
    protected:

    public:
        DDSAssetLoader() {
            pattern_ = R"((.+)\.dds)";
            type_ = AssetType::Texture2D;
        }

        std::shared_ptr<AssetBase> Load(const std::filesystem::path& path) override {
            if (!std::filesystem::exists(path))
                throw std::runtime_error("File does not exist: " + path.string());

            auto rs = Engine::Instance().RenderSystem();
            auto device = rs->GetRenderDevice().GetDxDevice();

            DirectX::ResourceUploadBatch upload_batch(device.get());
            upload_batch.Begin();

            std::shared_ptr<ID3D12Resource> texture = nullptr;

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

            return std::make_shared<GPULocalResource>(device, texture);
        }

        void Save(AssetBase& asset, std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving DDS textures is not supported.");
        }
    };
}
