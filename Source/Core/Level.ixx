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
import Component;
import Misc;

namespace GiiGa
{
    export class Level
    {
    
    public:

        Level(const Json::Value& level_root)
        {
            SetIsActive(false);
            
            auto level_settings = level_root["LevelSettings"];
            name = level_settings["Name"].asString();
            SetIsActive(false);

            auto root_level_go = level_settings["GameObjects"];
            for (auto&& gameobject_js : root_level_go)
            {
                AddRootGameObject(std::make_shared<GameObject>(gameobject_js));
            }
        }
        
        const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const
        {
            return root_game_objects_;
        }
        void AddRootGameObject(std::shared_ptr<GameObject> gameObject)
        {
            root_game_objects_.push_back(gameObject);
            for (const auto& component_ : gameObject->GetComponents())
            {
                componentsInLevel_.AddComponent(component_);
            }

            for (const auto& kid : gameObject->GetChildren())
            {
                for (const auto&& kid_comp : kid->getComponents())
                    componentsInLevel_.AddComponent(kid_comp);
            }
        }
        
        bool DestroyGameObject(std::shared_ptr<GameObject> gameObject)
        {
            Todo();// review
            for (const auto& component_ : gameObject->GetComponents())
                componentsInLevel_.removeComponent(component_);
            
            auto iterator = root_game_objects_.erase(
                    std::remove(root_game_objects_.begin(), root_game_objects_.end(), gameObject),
                    root_game_objects_.end());

            if (iterator->get())
            {
                return true;
            }
            return false;
        }
        bool GetIsActive() const
        {
            return isActive_;
        }
        
        void SetIsActive(bool newActive)
        {
            isActive_ = newActive;
        }
        
        template<typename T>
        std::vector<std::shared_ptr<T>> GetComponentsOfType()
        {
            return componentsInLevel_.getComponentsOfType<T>();
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
                throw std::runtime_error("Project file 'project.giga' not found in: " + level_path.string());
            }

            std::ifstream level_file(level_path);
            if (!level_file.is_open())
            {
                throw std::runtime_error("Failed to open project file: " + level_path.string());
            }
            
            Json::CharReaderBuilder reader_builder;
            Json::Value level_json;
            std::string errs;
            
            if (!Json::parseFromStream(reader_builder, level_file, &level_json, &errs))
            {
                throw std::runtime_error("Failed to parse project file: " + errs);
            }

            return Level(level_json);
        }
        
    public:
        std::string name;
        
    private:
        std::vector<std::shared_ptr<GameObject>> root_game_objects_;
        bool isActive_ = false;
        
        class 
        {
        public:
            template <typename T>
            void AddComponent(std::shared_ptr<T> component)
            {
                static_assert(std::is_base_of<Component, T>::value, "T must be derived from Component");
                components_[typeid(*component)].push_back(component);
            }

            template <typename T>
            std::vector<std::shared_ptr<T>> getComponentsOfType()
            {
                static_assert(std::is_base_of<Component, T>::value, "T must be derived from Component");

                std::type_index typeIndex(typeid(T));
                std::vector<std::shared_ptr<T>> result;

                auto it = components_.find(typeIndex);
                if (it != components_.end()) {
                    for (const auto& component : it->second) {
                        if (auto castedComponent = std::dynamic_pointer_cast<T>(component)) {
                            result.push_back(castedComponent);
                        }
                    }
                }

                return result;
            }
            
            void removeComponent(const std::shared_ptr<Component>& component) {
                // Будет работать только в том случае, если умные указатели будут корректно сравниваться в течении всей работы программы.
                // На короткой дистанции так скорее всего и будет.
                
                std::type_index typeIndex(typeid(*component));
                auto it = components_.find(typeIndex);
                if (it != components_.end()) {
                    auto& vec = it->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), component), vec.end());
                }
            }
    
        private:
            std::map<std::type_index, std::vector<std::shared_ptr<Component>>> components_;
        } componentsInLevel_; 
    };
}