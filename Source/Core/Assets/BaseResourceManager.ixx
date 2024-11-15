module;

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

export module BaseResourceManager;

import AssetHandle;
import AssetId;
import AssetType;
import AssetBase;
import BaseAssetDatabase;
import AssetLoader;
import Uuid;
import Misc;

namespace GiiGa
{

    export class BaseResourceManager
    {
    protected:
        BaseAssetDatabase* database_;

        std::unordered_map<AssetHandle, std::shared_ptr<AssetBase>> loaded_assets_;
        std::unordered_map<AssetType, std::vector<AssetLoader*>> asset_loaders_;

        virtual std::shared_ptr<AssetBase> GetAssetInternal(AssetHandle id) = 0;
    public:
        template <typename T>
        std::shared_ptr<T> GetAsset(AssetId<T> id) {
            Todo<std::shared_ptr<T>>();
        }
    };
}  // namespace GiiGa
