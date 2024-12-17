module;

#include <vector>
#include <memory>

export module ILevelRootGameObjects;

import IGameObject;

namespace GiiGa
{
    export class ILevelRootGameObjects
    {
    public:
        virtual ~ILevelRootGameObjects() = default;

        void AddRootGameObject(std::shared_ptr<IGameObject> rootGameObject)
        {
            root_game_objects_.push_back(rootGameObject);
        }

        void RemoveRootGameObject(std::shared_ptr<IGameObject> rootGameObject)
        {
            root_game_objects_.erase(
                std::remove(root_game_objects_.begin(), root_game_objects_.end(), rootGameObject),
                root_game_objects_.end());
        }

    protected:
        std::vector<std::shared_ptr<IGameObject>> root_game_objects_;
    };
}
