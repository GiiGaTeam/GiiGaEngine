#pragma once
#include<unordered_map>
#include<functional>
#include<memory>

#include<AssetHandle.h>
#include<AssetType.h>
#include<BaseAssetDatabase.h>
#include<AssetLoader.h>
#include<AssetBase.h>
#include<Uuid.h>

namespace GiiGa
{

    class ResourceManager
    {
    protected:
        friend class AssetBase;

        std::shared_ptr<BaseAssetDatabase> database_;

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

        void SetDatabase(std::shared_ptr<BaseAssetDatabase> database) {
            database_ = database;
        }

        std::shared_ptr<BaseAssetDatabase> Database() const {
            return database_;
        }

        //todo: actually if in game assets are not mutable, we don't need it in game, so...
        void UpdateAsset(const AssetHandle& handle)
        {
            if (loaded_assets_.contains(handle))
            {
                auto asset_meta_opt = database_->GetAssetMeta(handle, true);
                if (!asset_meta_opt)
                {
                    throw std::runtime_error("Asset metadata not found for handle: " + handle.id.ToString());
                }

                const AssetMeta& asset_meta = asset_meta_opt->get();
                auto loader_it = database_->asset_loader_by_uuid_.find(asset_meta.loader_id);
                if (loader_it == database_->asset_loader_by_uuid_.end())
                {
                    throw std::runtime_error("No loader available for asset type: " + AssetTypeToString(asset_meta.type));
                }

                loader_it->second->Update(loaded_assets_.at(handle).lock(), database_->asset_path_ / asset_meta.path);
            }
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
            auto asset_meta_opt = database_->GetAssetMeta(handle, true);
            if (!asset_meta_opt)
            {
                throw std::runtime_error("Asset metadata not found for handle: " + handle.id.ToString());
            }

            const AssetMeta& asset_meta = asset_meta_opt->get();
            auto loader_it = database_->asset_loader_by_uuid_.find(asset_meta.loader_id);
            if (loader_it == database_->asset_loader_by_uuid_.end())
            {
                throw std::runtime_error("No loader available for asset type: " + AssetTypeToString(asset_meta.type));
            }

            auto asset = loader_it->second->Load(handle, database_->asset_path_ / asset_meta.path);
            loaded_assets_[handle] = asset;
            asset->OnDestroy.Register([this](const auto& handle) {
                RemoveAsset(handle);
                });
            return std::dynamic_pointer_cast<T>(asset);
        }

        void RemoveAsset(AssetHandle handle) { 
            loaded_assets_.erase(handle);
        }
    };
}  // namespace GiiGa
