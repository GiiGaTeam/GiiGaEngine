module;

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

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

        // key - Path, value - Callback of loaded asset
        std::unordered_map<std::string, LoadCallback> callbacks_;

    public:
        std::shared_ptr<AssetBase> Load(std::string& path) { 
            Todo<std::shared_ptr<AssetBase>>();
        }

        void LoadAsync(std::string& path, LoadCallback&& callback) { 
            Todo<void>();
        }
    };
}  // namespace GiiGa
