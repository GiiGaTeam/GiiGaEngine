module;

#include <filesystem>
#include <span>
#include <concepts>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <iostream>
#include <easylogging++.h>

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

    public:
        EditorAssetDatabase(std::shared_ptr<Project> proj)
            : project_watcher_(std::vector<std::string>{
                  (proj->GetProjectPath() / "Assets").string()
              })
              , BaseAssetDatabase(proj->GetProjectPath())
        {
        }

        void StartProjectWatcher()
        {
            project_watcher_.OnFileAdded.Register([this](const auto& path)
            {
                try
                {
                    ImportAsset(path);
                }
                catch (std::runtime_error& err)
                {
                    el::Loggers::getLogger("ResourceManager")->warn("Failed to add new file: %v", err.what());
                }
            });

            project_watcher_.OnFileModified.Register([this](const auto& path)
            {
                //std::cout << "[DEBUG] File was modified: " << path << std::endl;
            });

            project_watcher_.OnFileRemoved.Register([this](const auto& path)
            {
                RemoveAsset(path);
            });

            project_watcher_.OnFileRenamed.Register([this](const auto& pair)
            {
                auto [f, s] = pair;
                UpdateAssetPath(f, s);
            });

            project_watcher_.OnFileMoved.Register([this](const auto& pair)
            {
                auto [f, s] = pair;
                UpdateAssetPath(f, s);
            });

            project_watcher_.StartWatch();
        }

        void Shutdown()
        {
            project_watcher_.ClearRemovedFiles();
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

        void ImportAsset(const std::filesystem::path& path)
        {
            if (std::filesystem::is_directory(asset_path_ / path))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(asset_path_ / path))
                {
                    if (entry.is_regular_file())
                    {
                        try
                        {
                            ImportAsset(std::filesystem::relative(entry.path(), asset_path_));
                        }
                        catch (std::runtime_error& err)
                        {
                            el::Loggers::getLogger("ResourceManager")->warn("Failed to add new file: %v", err.what());
                        }
                    }
                }
                return;
            }

            el::Loggers::getLogger("ResourceManager")->debug("Register new file: %v", path);

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

            for (auto& handle : handles)
            {
                AssetMeta meta;
                meta.path = path;
                meta.type = asset_type;

                registry_map_.emplace(handle, std::move(meta));
            }
        }

        void RemoveAsset(const std::filesystem::path& path)
        {
            auto it = registry_map_.begin();
            while (it != registry_map_.end())
            {
                const auto& asset_path = it->second.path;

                if (asset_path.string().compare(0, path.string().size(), path.string()) == 0)
                {
                    el::Loggers::getLogger("ResourceManager")->debug("File removed: %v", asset_path);
                    it = registry_map_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void UpdateAssetPath(const std::filesystem::path& old_path, const std::filesystem::path& new_path)
        {
            for (auto& [handle, meta] : registry_map_)
            {
                auto& asset_path = meta.path;

                if (asset_path.string().compare(0, old_path.string().size(), old_path.string()) == 0)
                {
                    std::filesystem::path relative_path = std::filesystem::relative(asset_path, old_path);

                    if (relative_path == ".")
                    {
                        asset_path = new_path;

                        el::Loggers::getLogger("ResourceManager")->debug("Updated path: %v -> %v", old_path, asset_path);
                    }
                    else
                    {
                        asset_path = new_path / relative_path;

                        el::Loggers::getLogger("ResourceManager")->debug("Updated path: %v -> %v", old_path / relative_path, asset_path);
                    }
                }
            }
        }
    };
} // namespace GiiGa
