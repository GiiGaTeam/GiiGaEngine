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
            return game_objects_;
        }
        void AddGameObject(std::shared_ptr<GameObject> game_object)
        {
            game_objects_.push_back(game_object);
        }
        bool DestroyGameObject(std::shared_ptr<GameObject> GameObject)
        {
            auto Iterator = game_objects_.erase(
                    std::remove(game_objects_.begin(), game_objects_.end(), GameObject),
                    game_objects_.end());

            if (Iterator->get())
            {
                return true;
            }
            return false;
        }
        bool GetIsActive() const
        {
            return is_active_;
        }
        
        void SetIsActive(bool newActive)
        {
            is_active_ = newActive;
        }
        
    public:
        std::string Name;
        
    private:
        std::vector<std::shared_ptr<GameObject>> game_objects_;
        bool is_active_ = false;
    };
}