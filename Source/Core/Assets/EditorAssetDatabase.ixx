module;

#include <filesystem>
#include <span>
#include <concepts>
#include <stdexcept>
#include <type_traits>

#include <iostream>

export module EditorAssetDatabase;

import BaseAssetDatabase;
import AssetHandle;
import AssetBase;
import AssetMeta;
import AssetLoader;
import ProjectWatcher;
import Project;

namespace GiiGa
{
    template <typename T>
    concept IsAssetBase = std::is_base_of_v<AssetBase, T>;

    export class EditorAssetDatabase : public BaseAssetDatabase
    {
    private:
        ProjectWatcher project_watcher_;
        std::filesystem::path asset_path_;

    public:
        EditorAssetDatabase(std::shared_ptr<Project> proj) 
            : project_watcher_(std::vector<std::string>{ 
                (proj->GetProjectPath() / "Assets").string()
            })
            , asset_path_(proj->GetProjectPath() / "Assets")
            , BaseAssetDatabase(proj->GetProjectPath())
        {
            
        }

        void StartProjectWatcher() {
            project_watcher_.OnFileAdded.Register([this](const auto& path) {
                std::cout << "[DEBUG] Register new file: " << path << std::endl;

                try {
                    ImportAsset(path);
                }
                catch (std::runtime_error& err) {
                    std::cout << "[WARN] Failed to add new file: " << err.what() << std::endl;
                }

                });

            project_watcher_.OnFileModified.Register([this](const auto& path) {
                //std::cout << "[DEBUG] File was modified: " << path << std::endl;
                });

            project_watcher_.OnFileRemoved.Register([this](const auto& path) {
                std::cout << "[DEBUG] File removed: " << path << std::endl;
                RemoveAsset(path);
                });

            project_watcher_.OnFileRenamed.Register([this](const auto& pair) {
                auto [f, s] = pair;
                std::cout << "[DEBUG] File was renamed from " << f << " to " << s << std::endl;
                UpdateAssetPath(f, s);

                });

            project_watcher_.OnFileMoved.Register([this](const auto& pair) {
                auto [f, s] = pair;
                std::cout << "[DEBUG] File was moved from " << f << " to " << s << std::endl;
                UpdateAssetPath(f, s);
                });

            project_watcher_.StartWatch();
        }

        template <IsAssetBase T>
        AssetHandle CreateAsset(T& asset, std::filesystem::path& path)
        {
            AssetHandle handle = asset.GetId();
            
            AssetMeta meta;
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

        void ImportAsset(const std::filesystem::path& path) {
            AssetType asset_type;
            AssetLoader* selected_loader = nullptr;
            bool found_loader = false;
            for (const auto& [type, loaders] : asset_loaders_)
            {
                for (const auto& loader : loaders)
                {
                    if (loader->MatchesPattern(path))
                    {
                        asset_type = type;
                        selected_loader = loader.get();
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

            auto handles = selected_loader->Preprocess(asset_path_ / path);

            for (auto& handle : handles) {
                AssetMeta meta;
                meta.path = path;
                meta.type = asset_type;

                registry_map_.emplace(handle, std::move(meta));
            }
        }

        void RemoveAsset(const std::filesystem::path& path) {
            auto it = std::find_if(registry_map_.begin(), registry_map_.end(), 
                [&path](const auto& pair) { 
                    return pair.second.path == path; 
                });

            if (it != registry_map_.end())
            {
                registry_map_.erase(it);
            }
        }

        void UpdateAssetPath(const std::filesystem::path& old_path, const std::filesystem::path& new_path){
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
