module;

#include <unordered_map>
#include <optional>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>

export module BaseAssetDatabase;

import AssetHandle;
import AssetMeta;
import AssetLoader;
import AssetType;

namespace GiiGa
{
    template <typename T>
    concept IsAssetLoader = std::is_base_of_v<AssetLoader, T>;

    export class BaseAssetDatabase
    {
    private:
        static inline const char* registry_name = "database.json";
    protected:
        std::filesystem::path registry_path_;
        std::fstream registry_file_;

        // key - AssetHandle, value - Asset meta
        std::unordered_map<AssetHandle, AssetMeta> registry_map_;

        std::unordered_map<AssetType, std::vector<std::shared_ptr<AssetLoader>>> asset_loaders_;
    public:
        friend class ResourceManager;

        BaseAssetDatabase(const std::filesystem::path& registry_path)
            : registry_path_(registry_path / registry_name)
        {}

        std::optional<std::reference_wrapper<AssetMeta>> GetAssetMeta(AssetHandle handle) { 
            std::cout << "[DEBUG] Request for asset: " << handle.id.ToString() << std::endl;

            auto it = registry_map_.find(handle);

            if (it != registry_map_.end())
            {
                std::cout << "[DEBUG] Found asset: " << handle.id.ToString() << std::endl;
                return std::ref(it->second);
            }

            return std::nullopt; 
        }

        void SaveRegistry()
        {
            std::cout << "[DEBUG] Saving registry" << std::endl;

            Json::Value root(Json::arrayValue);

            for (const auto& [handle, meta] : registry_map_)
            {
                Json::Value entry(Json::objectValue);
                entry["id"] = handle.ToJson();
                entry["meta"] = meta.ToJson();
                root.append(entry);
            }

            registry_file_ << root;
        }

        void InitializeDatabase()
        {
            OpenRegistryFile();

            if (registry_file_.peek() == std::ifstream::traits_type::eof())
            {
                registry_map_.clear();
                SaveRegistry();
            }
            else
            {
                LoadRegistry();
            }
        }

        template <IsAssetLoader T>
        void RegisterLoader()
        {
            auto loader_ptr = std::make_shared<T>();

            std::cout << "[DEBUG] Registered loader: " << loader_ptr->GetName() << std::endl;

            asset_loaders_[loader_ptr->Type()].emplace_back(std::move(loader_ptr));
        }
    private:
        virtual void _MakeVirtual() {

        }

        void LoadRegistry()
        {
            std::cout << "[DEBUG] Load registry" << std::endl;

            Json::Value root;
            registry_file_ >> root;

            if (!root.isArray())
            {
                throw std::runtime_error("Invalid registry file format: expected an array of entries.");
            }

            registry_map_.clear();

            for (const auto& entry : root)
            {
                if (!entry.isObject() || !entry.isMember("handle") || !entry.isMember("meta"))
                {
                    throw std::runtime_error("Invalid entry in registry file.");
                }

                AssetHandle handle = AssetHandle::FromJson(entry["handle"]);
                AssetMeta meta = AssetMeta::FromJson(entry["meta"]);

                registry_map_.emplace(handle, meta);
            }
        }

        void OpenRegistryFile()
        {
            std::cout << "[DEBUG] Opening or creating registry file" << std::endl;

            registry_file_.open(registry_path_, std::ios::in | std::ios::out);

            if (!registry_file_)
            {
                std::ofstream createFile(registry_path_);
                if (!createFile)
                {
                    throw std::runtime_error("Failed to create registry file: " + registry_path_.string());
                }
                createFile.close();

                registry_file_.open(registry_path_, std::ios::in | std::ios::out);
            }
        }
    };
}  // namespace GiiGa
