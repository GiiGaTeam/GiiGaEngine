#pragma once


#include<vector>
#include<memory>
#include<filesystem>
#include<json/json.h>

#include<AssetLoader.h>
#include<AssetMeta.h>
#include<AssetHandle.h>
#include<ConcreteAsset/PrefabAsset.h>
#include<GameObject.h>
#include<Logger.h>
#include<CreateComponentsForGameObject.h>
#include<IWorldQuery.h>

namespace GiiGa
{
    class PrefabAssetLoader : public AssetLoader
    {
    public:
        PrefabAssetLoader()
        {
            id_ = Uuid::FromString("a3227723-70bb-4b66-bab0-1d92d492a75c").value();
            pattern_ = R"((.+)\.(prefab))";
            type_ = AssetType::Prefab;
        }

        virtual ~PrefabAssetLoader() = default;

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
        {
            auto asset_meta = AssetMeta{
                type_,
                relative_path,
                id_,
                relative_path.stem().string()
            };
            
            return {
                std::make_pair(AssetHandle{Uuid::New(), 0}, asset_meta)
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

            std::unordered_map<Uuid, Uuid> prefab_uuid_to_world_uuid;
            std::vector<std::shared_ptr<GameObject>> created_game_objects;

            for (const auto& go_js : prefab_js["GameObjects"])
            {
                auto new_go = GameObject::CreateGameObjectFromJson(go_js, nullptr, true);
                CreateComponentsForGameObject::Create(new_go, go_js, prefab_uuid_to_world_uuid);
                created_game_objects.push_back(new_go);
                new_go->prefab_handle_ = handle;
                prefab_uuid_to_world_uuid[new_go->GetInPrefabUuid()] = new_go->GetUuid();
            }

            for (int i=0; i < created_game_objects.size(); i++)
            {
                created_game_objects[i]->RestoreAsPrefab(prefab_js["GameObjects"][i], prefab_uuid_to_world_uuid);
            }

            auto opt_root_js = Uuid::FromString(prefab_js["RootGameObject"].asString());

            if (!opt_root_js.has_value())
                throw std::runtime_error("Failed to parse Prefab root id");

            Uuid prefab_root_uuid = opt_root_js.value();
            Uuid world_root_uuid = prefab_uuid_to_world_uuid[prefab_root_uuid];
            
            auto root_go = WorldQuery::GetWithUUID<GameObject>(world_root_uuid);

            return std::make_shared<PrefabAsset>(handle, root_go);
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            auto prefab = std::dynamic_pointer_cast<PrefabAsset>(asset);

            el::Loggers::getLogger(LogWorld)->info("Saving Prefab %v in %v", prefab->GetId().id.ToString(), path);

            std::ofstream file(path, std::ios::out | std::ios::trunc);

            if (!file.is_open())
            {
                throw std::runtime_error("Failed to open project file for writing: " + path.string());
            }

            // Ensure ToJson() works correctly
            Json::Value json = prefab->ToJson();
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
