module;

#include <unordered_map>
#include <optional>
#include <string>
#include <filesystem>
#include <fstream>

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
        std::ofstream registry_file_;

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
    };
}  // namespace GiiGa
