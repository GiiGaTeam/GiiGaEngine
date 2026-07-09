#pragma once
#include <pybind11/conduit/wrap_include_python_h.h>

#include<stdexcept>
#include<filesystem>
#include<memory>

#include<AssetLoader.h>
#include<AssetType.h>
#include<ConcreteAsset/TextureAsset.h>
#include<TextureLoadUtils.h>
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

            nvrhi::TextureHandle texture = LoadTextureFileToNvrhi(rs->GetRenderDevice(), path, /*dds*/ false);

            return std::make_shared<TextureAsset>(handle, texture);
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
