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
    private:
        std::string name_;
        std::vector<std::shared_ptr<GameObject>> game_objects_;
    };
}