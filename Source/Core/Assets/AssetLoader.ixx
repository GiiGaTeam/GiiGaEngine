export module AssetLoader;

import <regex>;
import <unordered_map>;
import <string>;
import <functional>;
import <memory>;
import <filesystem>;
import <thread>;
import <vector>;

import AssetHandle;
import AssetType;
import AssetMeta;
import Uuid;
import Misc;

namespace GiiGa
{
    export class AssetBase;

    using LoadCallback = std::function<void(std::shared_ptr<AssetBase>)>;

    export class AssetLoader
    {
    protected:
        std::regex pattern_;
        AssetType type_;
        Uuid id_ = Uuid::Null();

    public:
        virtual ~AssetLoader() = default;
        virtual std::vector<std::pair<AssetHandle, AssetMeta>> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path) = 0;
        virtual std::shared_ptr<AssetBase> Load(AssetHandle handle, const std::filesystem::path& path) = 0;
        virtual void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) = 0;

        virtual void Update(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path)
        {
        }

        void LoadAsync(AssetHandle handle, const std::filesystem::path& path, LoadCallback&& callback)
        {
            std::thread([this, handle, path, callback = std::move(callback)]()
                {
                    try
                    {
                        auto asset = Load(handle, path);
                        callback(asset);
                    }
                    catch (...)
                    {
                        callback(nullptr);
                    }
                })
                .detach();
        }

        bool MatchesPattern(const std::filesystem::path& path) const
        {
            std::string filename = path.filename().string();
            return std::regex_match(filename, pattern_);
        }

        AssetType Type() const
        {
            return type_;
        }

        Uuid Id() const
        {
            return id_;
        }

        virtual const char* GetName()
        {
            return "Unnamed";
        }
    };
} // namespace GiiGa
