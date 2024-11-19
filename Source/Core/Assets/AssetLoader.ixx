module;

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <filesystem>
#include <thread>

export module AssetLoader;

import AssetHandle;
import AssetType;
import AssetBase;
import Uuid;
import Misc;

namespace GiiGa
{
    using LoadCallback = std::function<void(std::shared_ptr<AssetBase>)>;

    export class AssetLoader
    {
    protected:
        std::string pattern_;
        AssetType type_;

    public:
        virtual std::shared_ptr<AssetBase> Load(const std::filesystem::path& path) = 0;
        virtual void Save(std::filesystem::path& path) = 0;

        void LoadAsync(const std::filesystem::path& path, LoadCallback&& callback) { 
             std::thread([this, path, callback = std::move(callback)]() {
                try
                {
                    auto asset = Load(path);
                    callback(asset);
                }
                catch (...)
                {
                    callback(nullptr);
                }
            })
            .detach();
        }
    };
}  // namespace GiiGa
