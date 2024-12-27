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
                auto go_with_kids = root_go->ToJsonWithKids();
                
                result["RootGameObjects"].append(root_go->GetUuid().ToString());

                for (auto&& go : go_with_kids)
                {
                    result["GameObjects"].append(go);
                }
            }

            return result;
        }

        static std::shared_ptr<Level> LevelFromJson(const AssetHandle& handle, const Json::Value& json)
        {
            auto level = std::make_shared<Level>(handle, json["LevelSettings"]);

            // creates game objects only
            el::Loggers::getLogger(LogWorld)->info("Creating game objects, children, components");
            
            auto&& all_gos = json["GameObjects"];
            std::vector<std::shared_ptr<GameObject>> created_game_objects;
            for (auto&& gameobject_js : all_gos)
            {
                auto new_go = GameObject::CreateGameObjectFromJson(gameobject_js, level);
                CreateComponentsForGameObject(new_go, gameobject_js, false);
                created_game_objects.push_back(new_go);
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
                std::dynamic_pointer_cast<GameObject>(created_game_objects[i])->Restore(all_gos[i]);
            }

            return level;
        }

    private:
        std::string name_;
        bool isActive_ = false;
    };
}
