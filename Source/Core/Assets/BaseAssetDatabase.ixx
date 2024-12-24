export module BaseAssetDatabase;

import <unordered_map>;
import <optional>;
import <string>;
import <filesystem>;
import <fstream>;
import <iostream>;
import <json/json.h>;

import Logger;
import AssetHandle;
import AssetMeta;
import AssetLoader;
import AssetType;
import Uuid;

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
        std::filesystem::path asset_path_;

        std::unordered_map<AssetHandle, AssetMeta> registry_map_;
        std::unordered_map<std::filesystem::path, AssetHandle> assets_to_path_;

        std::unordered_map<AssetType, std::vector<std::shared_ptr<AssetLoader>>> asset_loaders_;
        std::unordered_map<AssetType, std::shared_ptr<AssetLoader>> asset_savers_;
        std::unordered_map<Uuid, std::shared_ptr<AssetLoader>> asset_loader_by_uuid_;

    public:
        friend class ResourceManager;

        BaseAssetDatabase(const std::filesystem::path& registry_path)
            : registry_path_(registry_path / registry_name)
              , asset_path_(registry_path / "Assets")
        {
        }

        std::optional<std::reference_wrapper<AssetMeta>> GetAssetMeta(AssetHandle handle)
        {
            el::Loggers::getLogger(LogResourceManager)->debug("Request for asset: %v",handle.id.ToString());

            auto it = registry_map_.find(handle);

            if (it != registry_map_.end())
            {
                el::Loggers::getLogger(LogResourceManager)->debug("Found asset: %v", handle.id.ToString());
                return std::ref(it->second);
            }

            return std::nullopt;
        }

        void SaveRegistry()
        {
            el::Loggers::getLogger(LogResourceManager)->debug("Saving registry");

            Json::Value root(Json::arrayValue);

            for (const auto& [handle, meta] : registry_map_)
            {
                Json::Value entry(Json::objectValue);
                entry["id"] = handle.ToJson();
                entry["meta"] = meta.ToJson();
                root.append(entry);
            }

            Json::StreamWriterBuilder writer_builder;
            std::ofstream registry_file_(registry_path_);
            registry_file_ << Json::writeString(writer_builder, root);
        }

        void Initialize()
        {
            OpenRegistryFile();
            LoadRegistry();
        }

        template <IsAssetLoader T>
        void RegisterLoader()
        {
            auto loader_ptr = std::make_shared<T>();

            el::Loggers::getLogger(LogResourceManager)->debug("Registered loader: %v", loader_ptr->GetName());

            asset_loaders_[loader_ptr->Type()].emplace_back(loader_ptr);
            asset_loader_by_uuid_.emplace(loader_ptr->Id(), loader_ptr);
        }

        const std::filesystem::path& AssetPath() const {
            return asset_path_;
        }

        AssetType IsRegisteredPath(const std::filesystem::path& path) const {
            auto it = assets_to_path_.find(path);
            if (it != assets_to_path_.end()) {
                auto handle_it = registry_map_.find(it->second);
                if (handle_it != registry_map_.end()) {
                    return handle_it->second.type;
                }
            }
            return AssetType::Unknown;
        }
    private:
        virtual void _MakeVirtual()
        {
        }

        void LoadRegistry()
        {
            el::Loggers::getLogger(LogResourceManager)->debug("Load registry %v", registry_path_);

            Json::Value root;
            Json::CharReaderBuilder reader_builder;
            std::string errs;

            std::ifstream registry_file_(registry_path_);
            if (!Json::parseFromStream(reader_builder, registry_file_, &root, &errs))
            {
                throw std::runtime_error("Failed to parse project file: " + errs);
            }

            if (!root.isArray())
            {
                throw std::runtime_error("Invalid registry file format: expected an array of entries.");
            }

            registry_map_.clear();

            for (const auto& entry : root)
            {
                if (!entry.isObject() || !entry.isMember("id") || !entry.isMember("meta"))
                {
                    throw std::runtime_error("Invalid entry in registry file.");
                }

                AssetHandle handle = AssetHandle::FromJson(entry["id"]);
                AssetMeta meta = AssetMeta::FromJson(entry["meta"]);

                auto handle_temp = handle;
                handle.subresource = 0;
                assets_to_path_.emplace(meta.path, handle);
                registry_map_.emplace(handle, meta);
            }
        }

        void OpenRegistryFile()
        {
            el::Loggers::getLogger(LogResourceManager)->debug("Opening or creating registry file %v", registry_path_);

            if (!std::filesystem::exists(registry_path_))
            {
                std::ofstream registry_file_(registry_path_);
                if (!registry_file_)
                {
                    throw std::runtime_error("Failed to create registry file: " + registry_path_.string());
                }

                el::Loggers::getLogger(LogResourceManager)->debug("Saving registry");

                Json::Value root(Json::arrayValue);

                Json::StreamWriterBuilder writer_builder;
                registry_file_ << Json::writeString(writer_builder, root);

                registry_file_.close();
            }
        }
    };
} // namespace GiiGa
