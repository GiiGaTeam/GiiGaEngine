module;

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

export module ResourceManager;

import AssetHandle;
import AssetType;
import BaseAssetDatabase;
import AssetLoader;
import AssetBase;
import Uuid;
import Misc;

namespace GiiGa
{

    export class ResourceManager
    {
    protected:
        friend class AssetBase;

        BaseAssetDatabase* database_;

        std::unordered_map<AssetHandle, std::weak_ptr<AssetBase>> loaded_assets_;
    public:
        template <typename T>
        std::shared_ptr<T> GetAsset(AssetHandle handle) {
            auto found_asset = FindAsset<T>(handle);

            if (found_asset)
            {
                return found_asset;
            }
            
            auto loaded_asset = LoadAsset<T>(handle);

            if (loaded_asset)
            {
                return loaded_asset;
            }

             throw std::runtime_error("Failed to load asset with handle: " + handle.id.ToString());
        }

    private:
        template <typename T>
        std::shared_ptr<T> FindAsset(AssetHandle handle) {
            auto it = loaded_assets_.find(handle);

            if (it != loaded_assets_.end()) {
                return std::dynamic_pointer_cast<T>(it->second.lock());
            }

            return nullptr;
        }

        template <typename T>
        std::shared_ptr<T> LoadAsset(AssetHandle handle) {
            auto asset_meta_opt = database_->GetAssetMeta(handle);
            if (!asset_meta_opt)
            {
                throw std::runtime_error("Asset metadata not found for handle: " + handle.id.ToString());
            }

            const AssetMeta& asset_meta = asset_meta_opt->get();
            auto loader_it = database_->asset_loaders_.find(asset_meta.type);
            if (loader_it == database_->asset_loaders_.end() || loader_it->second.empty())
            {
                throw std::runtime_error("No loader available for asset type: " + AssetTypeToString(asset_meta.type));
            }

            for (auto loader : loader_it->second)
            {
                if (loader->MatchesPattern(asset_meta.path))
                {
                    auto asset = loader->Load(handle, asset_meta.path);
                    loaded_assets_[handle] = asset;
                    asset->OnDestroy.Register([this](const auto& handle) {
                        RemoveAsset(handle);
                    });
                    return std::dynamic_pointer_cast<T>(asset);
                }
            }

            throw std::runtime_error("Failed to load asset with handle: " + handle.id.ToString() + " of type: " + AssetTypeToString(asset_meta.type));
        }

        void RemoveAsset(AssetHandle handle) { 
            loaded_assets_.erase(handle);
        }
    };
}  // namespace GiiGa
