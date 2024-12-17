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
import IComponentsInLevel;
import Component;
import Misc;

namespace GiiGa
{
    export class Level : public ILevelRootGameObjects, public IComponentsInLevel, public std::enable_shared_from_this<Level>
    {
    public:
        /*  Level Json:
         *  {
         *      LevelSettings:
         *      {
         *          Name:
         *      },
         *      GameObjects:[...]
         *  }
         */
        Level(const Json::Value& level_root)
        {
            SetIsActive(false);

            auto&& level_settings = level_root["LevelSettings"];
            name = level_settings["Name"].asString();

            auto&& root_level_go = level_root["GameObjects"];
            for (auto&& gameobject_js : root_level_go)
            {
                std::make_shared<GameObject>(gameobject_js, shared_from_this());
            }
        }

        const std::vector<std::shared_ptr<IGameObject>>& GetRootGameObjects() const
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

        template <typename T>
        std::vector<std::shared_ptr<T>> getComponentsOfType()
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");

            std::type_index typeIndex(typeid(T));
            std::vector<std::shared_ptr<T>> result;

            auto it = type_to_components_.find(typeIndex);
            if (it != type_to_components_.end())
            {
                for (const auto& component : it->second)
                {
                    if (auto castedComponent = std::dynamic_pointer_cast<T>(component))
                    {
                        result.push_back(castedComponent);
                    }
                }
            }

            return result;
        }

        Json::Value ToJson()
        {
            Json::Value result;
            result["Name"] = name;
            Json::Value gameObjectsJson;
            for (auto GO : root_game_objects_)
            {
                gameObjectsJson.append(GO->ToJson());
            }
            result["GameObjects"] = gameObjectsJson.toStyledString();
            return result;
        }

        static Level FromAbsolutePath(const std::filesystem::path& level_path)
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

            return Level(level_json);
        }

    private:
        std::string name;
        bool isActive_ = false;
    };
}
