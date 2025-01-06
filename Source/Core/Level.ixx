export module Level;

import <filesystem>;
import <string>;
import <vector>;
import <memory>;
import <typeindex>;
import <json/json.h>;
import <fstream>;
import <iostream>;

import GameObject;
import ILevelRootGameObjects;
import IWorldQuery;
import Component;
import Misc;
import Logger;
import AssetBase;
import PrefabAsset;
import CreateComponentsForGameObject;
import Engine;
import PrefabModifications;

namespace GiiGa
{
    export class Level : public ILevelRootGameObjects, public AssetBase
    {
    public:
        /*  Level Json:
         *  {
         *      LevelSettings:
         *      {
         *          Name:
         *      },
         *      GameObjects:
         *      [
         *          {
         *          GO1
         *          }
         *          {
         *          GO2
         *          }
         *      ]
         *  }
         */

        Level(const AssetHandle& handle, const Json::Value& level_settings):
            AssetBase(handle)
        {
            SetIsActive(false);

            name_ = level_settings["Name"].asString();
        }

        AssetType GetType() override
        {
            return AssetType::Level;
        }

        const auto& GetRootGameObjects() const
        {
            return root_game_objects_;
        }

        size_t GetNumRootGameObjects() const
        {
            return root_game_objects_.size();
        }

        const std::string& GetLevelName() const
        {
            return name_;
        }

        bool GetIsActive() const
        {
            return isActive_;
        }

        void SetIsActive(bool newActive)
        {
            isActive_ = newActive;
        }

        Json::Value ToJson()
        {
            Json::Value result;

            Json::Value level_settings;
            level_settings["Name"] = name_;
            result["LevelSettings"] = level_settings;

            for (auto&& [_,root_go] : root_game_objects_)
            {
                result["RootGameObjects"].append(root_go->GetUuid().ToString());

                std::vector<Json::Value> go_with_kids = RecurGOToJsonWithKids(std::dynamic_pointer_cast<GameObject>(root_go));

                for (auto&& go : go_with_kids)
                {
                    result["GameObjects"].append(go);
                }
            }

            return result;
        }

        static std::vector<Json::Value> RecurGOToJsonWithKids(std::shared_ptr<GameObject> go)
        {
            std::vector<Json::Value> jsons;

            // current game object is part of level 
            if (go->prefab_handle_ == AssetHandle{})
            {
                jsons.push_back(go->ToJsonWithComponents());

                for (auto&& kid : go->GetChildren())
                {
                    for (auto kid_kid_js : RecurGOToJsonWithKids(kid))
                    {
                        jsons.push_back(kid_kid_js);
                    }
                }
            }
            else // current game object is part of prefab -> save as prefab in level 
            {
                auto prefab = Engine::Instance().ResourceManager()->GetAsset<PrefabAsset>(go->prefab_handle_);
                jsons.push_back(PrefabAsset::GameObjectAsPrefabRoot(prefab, go));
            }

            return jsons;
        }

        static std::shared_ptr<Level> LevelFromJson(const AssetHandle& handle, const Json::Value& json)
        {
            auto level = std::make_shared<Level>(handle, json["LevelSettings"]);

            // creates game objects only
            el::Loggers::getLogger(LogWorld)->info("Creating game objects, children, components");

            auto&& all_gos_jsons = json["GameObjects"];
            std::vector<std::shared_ptr<GameObject>> created_game_objects;
            for (auto&& gameobject_js : all_gos_jsons)
            {
                if (gameobject_js["Prefab"].empty()) // is not prefab instance
                {
                    auto new_go = GameObject::CreateGameObjectFromJson(gameobject_js, level);
                    CreateComponentsForGameObject::Create(new_go, gameobject_js, false);
                    created_game_objects.push_back(new_go);
                }
                else
                {
                    auto prefab_handle = AssetHandle::FromJson(gameobject_js["Prefab"]);
                    auto prefab_asset = Engine::Instance().ResourceManager()->GetAsset<PrefabAsset>(prefab_handle);
                    PrefabModifications modifications{gameobject_js["Modifications"]};
                    prefab_asset->Instantiate(modifications, created_game_objects);
                }
            }

            for (auto&& root_go : json["RootGameObjects"])
            {
                std::string go_id_str = root_go.asString();
                Uuid go_id = Uuid::FromString(go_id_str).value();
                WorldQuery::GetWithUUID<GameObject>(go_id)->AttachToLevelRoot(level);
            }

            el::Loggers::getLogger(LogWorld)->info("Restoring components references");
            for (int i = 0; i < created_game_objects.size(); ++i)
            {
                if (all_gos_jsons[i]["Prefab"].empty()) // is not prefab instance
                {
                    std::dynamic_pointer_cast<GameObject>(created_game_objects[i])->Restore(all_gos_jsons[i]);
                }
                else
                {
                    PrefabModifications modifications{all_gos_jsons[i]["Modifications"]};
                    std::dynamic_pointer_cast<GameObject>(created_game_objects[i])->ApplyModifications(modifications);
                }
            }

            return level;
        }

    private:
        std::string name_;
        bool isActive_ = false;
    };
}
