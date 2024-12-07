module;

#include <filesystem>
#include <span>
#include <concepts>
#include <stdexcept>
#include <type_traits>

export module EditorAssetDatabase;

import BaseAssetDatabase;
import AssetHandle;
import AssetBase;
import AssetMeta;
import AssetLoader;

namespace GiiGa
{
    template <typename T>
    concept IsAssetBase = std::is_base_of_v<AssetBase, T>;

    export class EditorAssetDatabase : public BaseAssetDatabase
    {
    public:
        template <IsAssetBase T>
        AssetHandle CreateAsset(T& asset, std::filesystem::path& path)
        {
            AssetHandle handle = asset.GetId();
            
            AssetMeta meta;
            meta.id = handle;
            meta.path = path;

            registry_map_.emplace(handle, std::move(meta));

            // TODO: map of loaders for creating asset

            AssetType asset_type = handle.type;
            auto loaderIt = asset_loaders_.find(asset_type);

            if (loaderIt == asset_loaders_.end() || loaderIt->second.empty())
            {
                throw std::runtime_error("No asset loader found for asset type: " + AssetTypeToString(asset_type));
            }

            AssetLoader* loader = loaderIt->second.front();
            loader->Save(asset, path);

            return handle;
        }

        AssetHandle ImportAsset(std::filesystem::path& path) {
            AssetType asset_type;
            bool found_loader = false;
            for (const auto& [type, loaders] : asset_loaders_)
            {
                for (const auto& loader : loaders)
                {
                    if (loader->MatchesPattern(path))
                    {
                        asset_type = type;
                        found_loader = true;
                        break;
                    }
                }
                if (found_loader) break;
            }

            if (!found_loader)
            {
                throw std::runtime_error("No asset loader found for the file path: " + path.string());
            }

            AssetHandle handle{};
            handle.id = Uuid::New();
            handle.type = asset_type;

            AssetMeta meta;
            meta.id = handle;
            meta.path = path;

            registry_map_.emplace(handle, std::move(meta));

            return handle;
        }

        void RemoveAsset(std::filesystem::path& path) {
            auto it = std::find_if(registry_map_.begin(), registry_map_.end(), 
                [&path](const auto& pair) { 
                    return pair.second.path == path; 
                });

            if (it != registry_map_.end())
            {
                registry_map_.erase(it);
            }
        }

        void UpdateAssetPath(std::filesystem::path& old_path, std::filesystem::path&& new_path){
            auto it = std::find_if(registry_map_.begin(), registry_map_.end(), 
                [&old_path](const auto& pair) { 
                    return pair.second.path == old_path; 
                });

            if (it != registry_map_.end())
            {
                it->second.path = new_path;
            }
        }
    };
}  // namespace GiiGa
