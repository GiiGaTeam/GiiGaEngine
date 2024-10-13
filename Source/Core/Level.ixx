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
        std::vector<std::shared_ptr<GameObject>>& GetGameObjects()
        {
            return game_objects_;
        }
    private:
        std::string name_;
        std::vector<std::shared_ptr<GameObject>> game_objects_;
    };
}