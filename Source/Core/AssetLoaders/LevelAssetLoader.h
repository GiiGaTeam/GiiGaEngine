#pragma once


#include<memory>
#include<filesystem>
#include<json/json.h>

#include<AssetLoader.h>
#include<AssetMeta.h>
#include<AssetHandle.h>
#include<ConcreteAsset/PrefabAsset.h>
#include<ConcreteAsset/LevelAsset.h>
#include<Logger.h>
#include<Level.h>

namespace GiiGa
{
    class LevelAssetLoader : public AssetLoader
    {
    public:
        LevelAssetLoader()
        {
            id_ = Uuid::FromString("a2227623-70bb-4b66-bab0-1d92d492a75c").value();
            pattern_ = R"((.+)\.(level))";
            type_ = AssetType::Level;
        }
        virtual ~LevelAssetLoader() = default;

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
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
                throw std::runtime_error("Level file not found in: " + path.string());
            }

            std::ifstream level_file(path);
            if (!level_file.is_open())
            {
                throw std::runtime_error("Failed to open level file: " + path.string());
            }

            Json::CharReaderBuilder reader_builder;
            Json::Value level_json;
            std::string errs;

            if (!Json::parseFromStream(reader_builder, level_file, &level_json, &errs))
            {
                throw std::runtime_error("Failed to parse level file: " + errs);
            }
            
            return std::make_shared<LevelAsset>(handle, level_json);
        }

        void Save(::std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            auto level = std::dynamic_pointer_cast<LevelAsset>(asset);
            
            std::ofstream level_file(path, std::ios::out | std::ios::trunc);

            if (!level_file.is_open())
            {
                throw std::runtime_error("Failed to open project file for writing: " + path.string());
            }

            // Ensure ToJson() works correctly
            Json::Value json = level->json_;
            if (json.isNull())
            {
                throw std::runtime_error("Failed to convert object to JSON");
            }

            Json::StreamWriterBuilder writer_builder;
            std::string jsonString = Json::writeString(writer_builder, json);

            // Writing JSON string to the file
            level_file << jsonString;

            // Check if the file stream has any errors
            if (level_file.fail())
            {
                throw std::runtime_error("Failed to write to file: " + path.string());
            }

            level_file.close();
        }
    };
}
