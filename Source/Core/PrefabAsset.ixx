export module PrefabAsset;

import <json/value.h>;

import AssetBase;
import GameObject;
import TransformComponent;
import ConsoleComponent;
import StaticMeshComponent;

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
