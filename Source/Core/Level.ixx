module;

#include <string>
#include <vector>
#include <memory>
#include <json/json.h>

export module Level;

import GameObject;

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
        }
        bool DestroyGameObject(std::shared_ptr<GameObject> gameObject)
        {
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
    };
}