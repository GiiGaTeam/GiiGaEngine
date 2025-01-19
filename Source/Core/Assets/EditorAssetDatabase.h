#pragma once
#include<filesystem>
#include<stdexcept>
#include<type_traits>
#include<vector>
#include<mutex>

#include<Logger.h>
#include<BaseAssetDatabase.h>
#include<AssetHandle.h>
#include<AssetBase.h>
#include<AssetLoader.h>
#include<ProjectWatcher.h>
#include<Project.h>

namespace GiiGa
{
    template <typename T>
    concept IsAssetBase = std::is_base_of_v<AssetBase, T>;

    class EditorAssetDatabase : public BaseAssetDatabase
    {
    private:
        ProjectWatcher project_watcher_;

        std::mutex update_queue_mutex = {};
        // idk, i cant use unordered here -> internal compiler error
        std::vector<AssetHandle> update_queue_ = {};

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
                    el::Loggers::getLogger(LogResourceManager)->warn("Import Asset Callback: %v", path);
                    ImportAsset(path);
                }
                catch (std::runtime_error& err)
                {
                    el::Loggers::getLogger(LogResourceManager)->warn("Failed to add new file: %v", err.what());
                }
            });

            project_watcher_.OnFileModified.Register([this](const auto& path)
            {
                if (assets_to_path_.contains(path))
                {
                    std::lock_guard lock{update_queue_mutex};
                    if (std::find(update_queue_.begin(), update_queue_.end(), assets_to_path_.at(path)) == update_queue_.end())
                        update_queue_.emplace_back(assets_to_path_.at(path));
                    el::Loggers::getLogger(LogResourceManager)->info("Asset File %v was modified enqueue to update", path.string());
                }
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
        AssetHandle CreateAsset(std::shared_ptr<T> asset, const std::filesystem::path& relative_or_absolute_path, bool save = true)
        {
            AssetHandle handle = asset->GetId();

            auto loaderIt = asset_savers_.find(asset->GetType());

            if (loaderIt == asset_savers_.end())
            {
                throw std::runtime_error("No asset loader found for asset type: " + AssetTypeToString(asset->GetType()));
            }

            std::filesystem::path relative_path;
            std::filesystem::path absolute_path;
            if (relative_or_absolute_path.is_absolute())
            {
                relative_path = std::filesystem::relative(relative_or_absolute_path, asset_path_);
                absolute_path = relative_or_absolute_path;
            }
            if (relative_or_absolute_path.is_relative())
            {
                relative_path = relative_or_absolute_path;
                absolute_path = asset_path_ / relative_or_absolute_path;
            }

            AssetMeta meta;
            meta.path = relative_path;
            meta.type = asset->GetType();
            meta.loader_id = loaderIt->second->Id();
            meta.name = relative_or_absolute_path.stem().string();

            registry_map_.emplace(handle, std::move(meta));

            //TODO: Try catch, if save failed it should remove from registry_map and assets_to_path
            assets_to_path_.emplace(relative_path, handle);
            if (save)
                loaderIt->second->Save(asset, absolute_path);

            return handle;
        }

        template <IsAssetBase T>
        void SaveAsset(std::shared_ptr<T> asset)
        {
            auto meta_it = registry_map_.find(asset->GetId());

            if (meta_it == registry_map_.end())
            {
                throw std::runtime_error("Asset Was not registered");
            }

            AssetHandle handle = asset->GetId();
            AssetType asset_type = meta_it->second.type;

            auto saverIt = asset_savers_.find(asset_type);

            if (saverIt == asset_savers_.end())
            {
                throw std::runtime_error("No asset loader found for asset type: " + AssetTypeToString(asset_type));
            }

            AssetLoader* saver = saverIt->second.get();
            auto absolute_path = asset_path_ / meta_it->second.path;
            saver->Save(asset, absolute_path);
        }

        void ImportAsset(const std::filesystem::path& path)
        {
            //todo add better filtering
            if (path.string().find("__pycache__") != std::string::npos)
                return;

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
                            el::Loggers::getLogger(LogResourceManager)->warn("Failed to add new file: %v", err.what());
                        }
                    }
                }
                return;
            }

            if (assets_to_path_.contains(path))
            {
                return;
            }

            el::Loggers::getLogger(LogResourceManager)->debug("Register new file: %v", path);

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

            auto handles = selected_loader->Preprocess(asset_path_ / path, path);

            for (auto& [handle, meta] : handles)
            {
                registry_map_.emplace(handle, std::move(meta));
            }

            auto handle = handles.begin()->first;
            handle.subresource = 0;
            assets_to_path_.emplace(path, handle);
        }

        void RemoveAsset(const std::filesystem::path& path)
        {
            auto it = registry_map_.begin();
            while (it != registry_map_.end())
            {
                const auto& asset_path = it->second.path;

                if (asset_path.string().compare(0, path.string().size(), path.string()) == 0)
                {
                    el::Loggers::getLogger(LogResourceManager)->debug("File removed: %v", asset_path);

                    assets_to_path_.erase(it->second.path);
                    it = registry_map_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        std::filesystem::path GetAssetAbsolutePath(const std::shared_ptr<AssetBase> asset)
        {
            auto reg_it = registry_map_.find(asset->GetId());
            if (reg_it != registry_map_.end())
            {
                return asset_path_ / reg_it->second.path;
            }
        }

        void RemoveAssetFile(std::shared_ptr<AssetBase> asset)
        {
            auto path = GetAssetAbsolutePath(asset);
            std::filesystem::remove(path);
            RemoveAsset(path);
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

                        assets_to_path_.erase(old_path);
                        auto handle_temp = handle;
                        handle_temp.subresource = 0;
                        assets_to_path_.emplace(asset_path, handle);

                        el::Loggers::getLogger(LogResourceManager)->debug("Updated path: %v -> %v", old_path, asset_path);
                    }
                    else
                    {
                        asset_path = new_path / relative_path;

                        assets_to_path_.erase(old_path / relative_path);
                        auto handle_temp = handle;
                        handle_temp.subresource = 0;
                        assets_to_path_.emplace(asset_path, handle);

                        el::Loggers::getLogger(LogResourceManager)->debug("Updated path: %v -> %v", old_path / relative_path, asset_path);
                    }
                }
            }
        }

        void ScanAssetsFolderForNewFiles()
        {
            try
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(asset_path_))
                {
                    if (entry.is_regular_file())
                    {
                        auto relative_path = std::filesystem::relative(entry.path(), asset_path_);

                        if (!IsFileInRegistry(relative_path))
                        {
                            el::Loggers::getLogger(LogResourceManager)->debug("Found new file: %v", relative_path);

                            try
                            {
                                ImportAsset(relative_path);
                            }
                            catch (std::runtime_error& err)
                            {
                                el::Loggers::getLogger(LogResourceManager)->warn("Failed to add new file: %v", err.what());
                            }
                        }
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                el::Loggers::getLogger(LogResourceManager)->warn("Failed to scan Assets folder: %v", e.what());
            }
        }

        void RemoveMissingFilesFromRegistry()
        {
            for (auto it = registry_map_.begin(); it != registry_map_.end();)
            {
                const auto& relative_path = it->second.path;
                auto absolute_path = asset_path_ / relative_path;

                if (!std::filesystem::exists(absolute_path))
                {
                    el::Loggers::getLogger(LogResourceManager)->debug("File missing, removing from registry: %v", relative_path);
                    assets_to_path_.erase(it->second.path);
                    it = registry_map_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        auto GetUpdateQueue()
        {
            return std::pair{&update_queue_mutex, &update_queue_};
        }

    private:
        bool IsFileInRegistry(const std::filesystem::path& relative_path) const
        {
            for (const auto& [handle, meta] : registry_map_)
            {
                if (meta.path == relative_path)
                {
                    return true;
                }
            }
            return false;
        }
    };
} // namespace GiiGa
