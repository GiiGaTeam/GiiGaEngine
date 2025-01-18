#pragma once


#include<string>
#include<vector>
#include<memory>

#include<GameObject.h>
#include<ILevelRootGameObjects.h>
#include<IWorldQuery.h>
#include<Component.h>
#include<Logger.h>
#include<AssetBase.h>
#include<ConcreteAsset/PrefabAsset.h>
#include<CreateComponentsForGameObject.h>
#include<Engine.h>
#include<PrefabInstanceModifications.h>
#include<ConcreteAsset/LevelAsset.h>

namespace GiiGa
{
    class Level : public ILevelRootGameObjects
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

        Level(const Json::Value& level_settings)
        {
            SetIsActive(false);
            name_ = level_settings["Name"].asString();
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

        void BeginPlay()
        {
            for (int i = 0; i < root_game_objects_.size(); ++i)
            {
                std::dynamic_pointer_cast<GameObject>(root_game_objects_[i])->BeginPlay();
            }
        }

        Json::Value ToJson() const
        {
            Json::Value result;

            Json::Value level_settings;
            level_settings["Name"] = name_;
            result["LevelSettings"] = level_settings;

            for (auto&& root_go : root_game_objects_)
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
                jsons.push_back(PrefabAsset::GameObjectAsPrefabInstance(prefab, go));
            }

            return jsons;
        }

        static std::shared_ptr<Level> LevelFromLevelAsset(std::shared_ptr<LevelAsset> level_asset)
        {
            auto level = LevelFromJson(level_asset->json_);
            level->asset_ = level_asset;
            return level;
        }

        std::shared_ptr<LevelAsset> GetLevelAsset() const
        {
            return asset_;
        }

        std::shared_ptr<LevelAsset> CreateAndReplaceLevelAsset()
        {
            Json::Value level_json = ToJson();
            auto asset = std::make_shared<LevelAsset>(asset_->GetId(), level_json);
            this->asset_ = asset;
            return asset;
        }

    private:
        std::shared_ptr<LevelAsset> asset_;
        std::string name_;
        bool isActive_ = false;

        static std::shared_ptr<Level> LevelFromJson(const Json::Value& json)
        {
            auto level = std::make_shared<Level>(json["LevelSettings"]);

            // creates game objects only
            el::Loggers::getLogger(LogWorld)->info("Creating game objects, children, components");

            auto&& all_gos_jsons = json["GameObjects"];
            std::vector<std::shared_ptr<GameObject>> created_game_objects;
            std::unordered_map<std::shared_ptr<PrefabAsset>,
                               std::vector<std::pair<std::shared_ptr<GameObject>, PrefabInstanceModifications>>> created_prefab_instances;
            for (auto&& gameobject_js : all_gos_jsons)
            {
                if (gameobject_js["Prefab"].empty()) // is not prefab instance
                {
                    auto new_go = GameObject::CreateGameObjectFromJson(gameobject_js);
                    CreateComponentsForGameObject::Create(new_go, gameobject_js, false);
                    created_game_objects.push_back(new_go);
                }
                else
                {
                    auto prefab_handle = AssetHandle::FromJson(gameobject_js["Prefab"]);
                    auto prefab_asset = Engine::Instance().ResourceManager()->GetAsset<PrefabAsset>(prefab_handle);
                    PrefabInstanceModifications modifications{gameobject_js["InstanceModifications"]};
                    auto instance = prefab_asset->Instantiate(modifications.InPrefabUuid_to_Instance, modifications.Removed_GOs_Comps);
                    created_prefab_instances[prefab_asset].push_back({instance, modifications});
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
                    std::dynamic_pointer_cast<GameObject>(created_game_objects[i])->RestoreFromLevelJson(all_gos_jsons[i]);
                }
            }

            for (auto&& prefab_assets : created_prefab_instances)
            {
                for (auto&& instance : prefab_assets.second)
                {
                    instance.first->ApplyModifications(instance.second.PropertyModifications);
                }
            }

            return level;
        }
    };
}
