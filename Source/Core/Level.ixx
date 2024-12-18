module;

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>
#include <fstream>

export module Level;

import GameObject;
import ILevelRootGameObjects;
import IWorldQuery;
import Component;
import Misc;

namespace GiiGa
{
    export class Level : public ILevelRootGameObjects
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

            name = level_settings["Name"].asString();
        }

        const auto& GetRootGameObjects() const
        {
            return root_game_objects_;
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
            result["Name"] = name;
            Json::Value gameObjectsJson;
            Todo();
            for (auto&& [_,GO] : root_game_objects_)
            {
                gameObjectsJson.append(GO->ToJson());
            }
            result["GameObjects"] = gameObjectsJson.toStyledString();
            return result;
        }

        static std::shared_ptr<Level> FromAbsolutePath(const std::filesystem::path& level_path)
        {
            if (!std::filesystem::exists(level_path))
            {
                throw std::runtime_error("Level file not found in: " + level_path.string());
            }

            std::ifstream level_file(level_path);
            if (!level_file.is_open())
            {
                throw std::runtime_error("Failed to open level file: " + level_path.string());
            }

            Json::CharReaderBuilder reader_builder;
            Json::Value level_json;
            std::string errs;

            if (!Json::parseFromStream(reader_builder, level_file, &level_json, &errs))
            {
                throw std::runtime_error("Failed to parse level file: " + errs);
            }

            auto level = std::make_shared<Level>(level_json["LevelSettings"]);

            // creates game objects with their components
            std::vector<std::shared_ptr<GameObject>> gameobjects;
            auto&& level_gos = level_json["GameObjects"];
            for (auto&& gameobject_js : level_gos)
            {
                auto new_go = std::make_shared<GameObject>(gameobject_js, level);
                new_go->RegisterInWorld();
                gameobjects.push_back(new_go);
            }

            for (auto it = level_gos.begin(); it != level_gos.end(); ++it)
            {
                gameobjects[it.index()]->Restore(*it);
            }

            return level;
        }

    private:
        std::string name;
        bool isActive_ = false;
    };
}
