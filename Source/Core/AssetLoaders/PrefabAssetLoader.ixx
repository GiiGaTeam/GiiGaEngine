export module PrefabAssetLoader;

import<vector>;
import<memory>;
import <filesystem>;
import <json/json.h>;

import AssetLoader;
import AssetMeta;
import AssetHandle;
import PrefabAsset;
import GameObject;
import Logger;

namespace GiiGa
{
    export class PrefabAssetLoader : public AssetLoader
    {
    public:
        virtual ~PrefabAssetLoader() = default;

        virtual std::vector<std::pair<AssetHandle, AssetMeta>> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
        {
            return {
                std::make_pair(AssetHandle{Uuid::New(), 0}, AssetMeta{
                                   type_,
                                   relative_path,
                                   id_,
                                   relative_path.stem().string()
                               })
            };
        }

        std::shared_ptr<AssetBase> Load(::GiiGa::AssetHandle handle, const ::std::filesystem::path& path) override
        {
            if (!std::filesystem::exists(path))
            {
                auto msg = "Prefab file not found in: " + path.string();
                el::Loggers::getLogger(LogResourceManager)->warn(msg);
                throw std::runtime_error(msg);
            }

            std::ifstream prefab_file(path);
            if (!prefab_file.is_open())
            {
                auto msg = "Failed to open Prefab file: " + path.string();
                el::Loggers::getLogger(LogResourceManager)->warn(msg);
                throw std::runtime_error(msg);
            }

            Json::CharReaderBuilder reader_builder;
            Json::Value prefab_js;
            std::string errs;

            if (!Json::parseFromStream(reader_builder, prefab_file, &prefab_js, &errs))
                throw std::runtime_error("Failed to parse Prefab file: " + errs);

            return std::make_shared<PrefabAsset>(handle, GameObject::CreateGameObjectFromJson(prefab_js, nullptr, true));
        }

        void Save(std::shared_ptr<AssetBase> asset, ::std::filesystem::path& path) override
        {
            auto prefab = std::dynamic_pointer_cast<PrefabAsset>(asset);

            el::Loggers::getLogger(LogWorld)->info("Saving Prefab %v in %v", prefab->GetId().id.ToString(), path);

            std::ofstream file(path, std::ios::out | std::ios::trunc);

            if (!file.is_open())
            {
                throw std::runtime_error("Failed to open project file for writing: " + path.string());
            }

            // Ensure ToJson() works correctly
            Json::Value json = prefab->root->ToJson();
            if (json.isNull())
            {
                throw std::runtime_error("Failed to convert object to JSON");
            }

            Json::StreamWriterBuilder writer_builder;
            std::string jsonString = Json::writeString(writer_builder, json);

            // Writing JSON string to the file
            file << jsonString;

            // Check if the file stream has any errors
            if (file.fail())
            {
                throw std::runtime_error("Failed to write to file: " + path.string());
            }

            file.close();
        }
    };
}
