export module PrefabAsset;

import <json/value.h>;
import <memory>;
import <optional>;

import AssetBase;
import GameObject;
import TransformComponent;
import ConsoleComponent;
import StaticMeshComponent;
import Engine;
import PrefabModifications;

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

            std::vector<Json::Value> go_with_kids = RecurGOToJsonWithKids(std::dynamic_pointer_cast<GameObject>(root), true);

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
            
            PrefabModifications modifications;
            gameObject->FindModifications(modifications, prefab->root);
            root["Modifications"] = modifications.ToJson();

            return root;
        }

        std::shared_ptr<GameObject> Clone(std::optional<PrefabModifications> modifications)
        {
            std::unordered_map<Uuid, Uuid> prefab_uuid_to_world;
            auto new_go = root->Clone(prefab_uuid_to_world, modifications);
            new_go->RestoreFromOriginal(root, prefab_uuid_to_world);
            return new_go;
        }

        std::vector<Json::Value> RecurGOToJsonWithKids(std::shared_ptr<GameObject> go, bool is_root = false)
        {
            std::vector<Json::Value> jsons;

            // saving new prefab OR editing/saving existing prefab with mod
            if (go->prefab_handle_ == AssetHandle{} || go->prefab_handle_ == GetId())
            {
                jsons.push_back(go->ToJsonWithComponents(is_root));

                for (auto&& kid : go->GetChildren())
                {
                    for (auto kid_kid_js : RecurGOToJsonWithKids(kid))
                    {
                        jsons.push_back(kid_kid_js);
                    }
                }
            }
            else // current go is part of other prefab -- save as sub prefab
            {
                auto sub_prefab = Engine::Instance().ResourceManager()->GetAsset<PrefabAsset>(go->prefab_handle_);
                jsons.push_back(GameObjectAsPrefabRoot(sub_prefab, go));
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
