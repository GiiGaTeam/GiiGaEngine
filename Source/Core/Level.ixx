module;

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>
#include <fstream>
#include <iostream>

export module Level;

import GameObject;
import ILevelRootGameObjects;
import IWorldQuery;
import Component;
import Misc;
import Logger;

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

            name_ = level_settings["Name"].asString();
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

        void SaveToAbsolutePath(const std::filesystem::path& level_path)
        {
            el::Loggers::getLogger(LogWorld)->info("Saving Level %v in %v", this->name_, level_path);

            std::ofstream level_file(level_path, std::ios::out | std::ios::trunc);

            if (!level_file.is_open())
            {
                throw std::runtime_error("Failed to open project file for writing: " + level_path.string());
            }

            // Ensure ToJson() works correctly
            Json::Value json = ToJson();
            if (json.isNull())
            {
                throw std::runtime_error("Failed to convert object to JSON");
            }

            Json::StreamWriterBuilder writer_builder;
            std::string jsonString = Json::writeString(writer_builder, json);

            // Writing JSON string to the file
            level_file << jsonString;

            // Check if the file stream has any errors
            if (level_file.fail())
            {
                throw std::runtime_error("Failed to write to file: " + level_path.string());
            }

            level_file.close();
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

            // creates game objects only
            el::Loggers::getLogger(LogWorld)->info("Creating game objects, children, components");
            auto&& level_root_gos = level_json["RootGameObjects"];
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
