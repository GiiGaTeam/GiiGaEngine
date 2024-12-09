module;

#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>

export module Level;

import GameObject;
import Component;

namespace GiiGa
{
    export class Level
    {
    
    public:
        const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const
        {
            return gameObjects_;
        }
        void AddGameObject(std::shared_ptr<GameObject> gameObject)
        {
            gameObjects_.push_back(gameObject);
            for (const auto& component_ : gameObject->GetComponents())
            {
                componentsInLevel_.AddComponent(component_);
            }
        }
        bool DestroyGameObject(std::shared_ptr<GameObject> gameObject)
        {
            for (const auto& component_ : gameObject->GetComponents())
                componentsInLevel_.removeComponent(component_);
            
            auto iterator = gameObjects_.erase(
                    std::remove(gameObjects_.begin(), gameObjects_.end(), gameObject),
                    gameObjects_.end());

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
            for (auto GO : gameObjects_)
            {
                gameObjectsJson.append(GO->ToJson());
            }
            result["GameObjects"] = gameObjectsJson.toStyledString();
            return result;
        }

        // todo
        void fromJson(const Json::Value& value);
        
    public:
        std::string name;
        
    private:
        std::vector<std::shared_ptr<GameObject>> gameObjects_;
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