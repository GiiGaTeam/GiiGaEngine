module;

#include <string>
#include <vector>
#include <memory>

export module Level;

import GameObject;

namespace GiiGa
{
    export class Level
    {
    public:
        const std::vector<std::shared_ptr<GameObject>>& GetGameObjects()
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
        
    public:
        std::string name;
        
    private:
        std::vector<std::shared_ptr<GameObject>> gameObjects_;
        bool isActive_ = false;
    };
}