module;

#include <unordered_map>
#include <optional>
#include <string>
#include <filesystem>
#include <fstream>
#include <json/json.h>

export module BaseAssetDatabase;

import AssetHandle;
import AssetMeta;
import AssetLoader;
import AssetType;

namespace GiiGa
{
    export class BaseAssetDatabase
    {
    protected:
        std::filesystem::path registry_path_;
        std::fstream registry_file_;

        // key - AssetHandle, value - Asset meta
        std::unordered_map<AssetHandle, AssetMeta> registry_map_;

        std::unordered_map<AssetType, std::vector<AssetLoader*>> asset_loaders_;
    public:
        friend class ResourceManager;

        void OpenRegistryFile()
        {
            if (!std::filesystem::exists(registry_path_))
            {
                std::ofstream createFile(registry_path_);
                if (!createFile)
                {
                    throw std::runtime_error("Failed to create registry file: " + registry_path_.string());
                }
                createFile.close();
            }

            registry_file_.open(registry_path_, std::ios::in | std::ios::out);

            if (!registry_file_)
            {
                throw std::runtime_error("Failed to open registry file: " + registry_path_.string());
            }
        }

        std::optional<std::reference_wrapper<AssetMeta>> GetAssetMeta(AssetHandle handle) { 
            auto it = registry_map_.find(handle);

            if (it != registry_map_.end())
            {
                return std::ref(it->second);
            }

            return std::nullopt; 
        }

        void SaveRegistry()
        {
            Json::Value root;

            for (const auto& [handle, meta] : registry_map_)
            {
                Json::Value entry;
                entry["id"] = handle.ToJson();
                entry["meta"] = meta.ToJson();
                root.append(entry);
            }

            registry_file_ << root;
        }

        void LoadRegistry()
        {
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
    };
}  // namespace GiiGa
