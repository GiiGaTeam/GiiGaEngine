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
import Misc;

namespace GiiGa
{
    export class BaseAssetDatabase
    {
    protected:
        std::filesystem::path registry_path_;
        std::fstream registry_file_;

        // key - AssetHandle, value - Asset meta
        std::unordered_map<AssetHandle, AssetMeta> registry_map_;

    public:
        void AddDependency(AssetHandle asset, AssetHandle dependency) { 
            auto assetMetaOpt = GetAssetMeta(asset);
            auto dependencyMetaOpt = GetAssetMeta(dependency);

            if (assetMetaOpt && dependencyMetaOpt)
            {
                auto& assetMeta = assetMetaOpt->get();
                auto& dependencyMeta = dependencyMetaOpt->get();

                if (std::find(assetMeta.dependencies.begin(), assetMeta.dependencies.end(), dependency) == assetMeta.dependencies.end())
                {
                    assetMeta.dependencies.push_back(dependency);
                }

                if (std::find(dependencyMeta.related.begin(), dependencyMeta.related.end(), asset) == dependencyMeta.related.end())
                {
                    dependencyMeta.related.push_back(asset);
                }
            }
        }

        void RemoveDependency(AssetHandle asset, AssetHandle dependency) {
            auto assetMetaOpt = GetAssetMeta(asset);
            auto dependencyMetaOpt = GetAssetMeta(dependency);

            if (assetMetaOpt && dependencyMetaOpt)
            {
                auto& assetMeta = assetMetaOpt->get();
                auto& dependencyMeta = dependencyMetaOpt->get();

                assetMeta.dependencies.erase(
                    std::remove(assetMeta.dependencies.begin(), assetMeta.dependencies.end(), dependency), assetMeta.dependencies.end());

                dependencyMeta.related.erase(
                    std::remove(dependencyMeta.related.begin(), dependencyMeta.related.end(), asset), dependencyMeta.related.end());
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
    };
}  // namespace GiiGa
