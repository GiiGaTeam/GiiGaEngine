module;

#include <unordered_map>
#include <memory>

export module ILevelRootGameObjects;

import IGameObject;

namespace GiiGa
{
    export class ILevelRootGameObjects : public std::enable_shared_from_this<ILevelRootGameObjects>
    {
    public:
        virtual ~ILevelRootGameObjects() = default;

        void AddRootGameObject(std::shared_ptr<IGameObject> rootGameObject)
        {
            root_game_objects_.insert({rootGameObject->GetUuid(), rootGameObject});
        }

        void RemoveRootGameObject(std::shared_ptr<IGameObject> rootGameObject)
        {
            root_game_objects_.erase(rootGameObject->GetUuid());
        }

    protected:
        std::unordered_map<Uuid, std::shared_ptr<IGameObject>> root_game_objects_;
    };
}
