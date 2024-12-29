export module PrefabAsset;

import <json/value.h>;
import <memory>;

import AssetBase;
import GameObject;
import TransformComponent;
import ConsoleComponent;
import StaticMeshComponent;

namespace GiiGa
{
    export class PrefabAsset final : public AssetBase, std::enable_shared_from_this<PrefabAsset>
    {
    public:
        PrefabAsset(AssetHandle handle, std::shared_ptr<GameObject> in_root = nullptr):
            AssetBase(handle), root(in_root)
        {
        }

        Json::Value ToJson()
        {
            Json::Value result;
            result["RootGameObject"] = root->GetUuid().ToString();

            std::vector<Json::Value> go_with_kids = RecurGOToJsonWithKids(std::dynamic_pointer_cast<GameObject>(root));

            for (auto&& go : go_with_kids)
            {
                result["GameObjects"].append(go);
            }

            return result;
        }
        
        static Json::Value GameObjectAsPrefabRoot(std::shared_ptr<PrefabAsset> prefab, std::shared_ptr<GameObject> gameObject)
        {
            Json::Value root;

            root["Prefab"] = prefab->GetId().ToJson();
            auto modifications = gameObject->FindModifications(prefab->root);

            for (const auto& modification : modifications)
            {
                root["Modifications"].append(modification);
            }

            return root;
        }

        std::shared_ptr<GameObject> Clone()
        {
            std::unordered_map<Uuid, Uuid> prefab_uuid_to_world;
            return root->Clone(prefab_uuid_to_world);
        }

        std::vector<Json::Value> RecurGOToJsonWithKids(std::shared_ptr<GameObject> go)
        {
            std::vector<Json::Value> jsons;
            
            if (go->GetPrefabHandle() == AssetHandle{} || go->GetPrefabHandle() == GetId())
            {
                jsons.push_back(go->ToJsonWithComponents());

                for (auto&& [_,kid] : go->GetChildren())
                {
                    for (auto kid_kid_js : RecurGOToJsonWithKids(kid))
                    {
                        jsons.push_back(kid_kid_js);
                    }
                }
            }
            else
            {
                jsons.push_back(GameObjectAsPrefabRoot(shared_from_this(), go));
            }

            return jsons;
        }

        std::shared_ptr<GameObject> root;

        ~PrefabAsset() override = default;

        AssetType GetType() override
        {
            return AssetType::Prefab;
        }
    };
}