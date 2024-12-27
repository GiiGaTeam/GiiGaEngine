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
                auto go_with_kids = root_go->ToJson();
                el::Loggers::getLogger(LogWorld)->debug("game object: %v", go_with_kids.toStyledString());
                result["RootGameObjects"].append(go_with_kids);
            }

            return result;
        }

        static std::shared_ptr<Level> LevelFromJson(const AssetHandle& handle,const Json::Value& json)
        {
            auto level = std::make_shared<Level>(handle, json["LevelSettings"]);

            // creates game objects only
            el::Loggers::getLogger(LogWorld)->info("Creating game objects, children, components");
            auto&& level_root_gos = json["RootGameObjects"];
            for (auto&& gameobject_js : level_root_gos)
            {
                auto new_go = GameObject::CreateGameObjectFromJson(gameobject_js, level);
                new_go->AttachToLevelRoot(level);
            }

            el::Loggers::getLogger(LogWorld)->info("Restoring components references");
            for (auto&& gameobject_js : level_root_gos)
            {
                auto root_go = level->root_game_objects_.at(Uuid::FromString(gameobject_js["Uuid"].asString()).value());
                std::dynamic_pointer_cast<GameObject>(root_go)->Restore(gameobject_js);
            }

            return level;
        }

    private:
        std::string name_;
        bool isActive_ = false;
    };
}
