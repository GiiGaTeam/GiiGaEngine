export module PrefabAsset;

import <json/value.h>;

import AssetBase;
import GameObject;
import TransformComponent;
import ConsoleComponent;
import StaticMeshComponent;

namespace GiiGa
{
    export void CreateComponentsForGameObject(std::shared_ptr<GameObject> gameObject, const Json::Value& go_js, bool roll_id = false)
    {
        for (auto&& comp_js : go_js["Components"])
        {
            if (comp_js["Type"].asString() == typeid(TransformComponent).name())
            {
                gameObject->transform_ = gameObject->CreateComponent<TransformComponent>(comp_js, roll_id);
            }
            else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
            {
                gameObject->CreateComponent<ConsoleComponent>(comp_js, roll_id);
            }
            else if (comp_js["Type"].asString() == typeid(StaticMeshComponent).name())
            {
                gameObject->CreateComponent<StaticMeshComponent>(comp_js, roll_id);
            }
        }
    }

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
