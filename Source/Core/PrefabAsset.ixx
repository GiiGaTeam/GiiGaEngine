export module PrefabAsset;

import AssetBase;
import GameObject;

namespace GiiGa
{
    export class PrefabAsset final : public AssetBase
    {
    public:
        PrefabAsset(AssetHandle handle, std::shared_ptr<GameObject> in_root = nullptr):
            AssetBase(handle), root(in_root)
        {
        }

        std::shared_ptr<GameObject> root;

        ~PrefabAsset() override = default;

        AssetType GetType() override
        {
            return AssetType::Prefab;
        }
    };
}
