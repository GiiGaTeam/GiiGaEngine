#pragma once


#include<unordered_map>
#include<memory>

#include<IGameObject.h>

namespace GiiGa
{
    class ILevelRootGameObjects : public std::enable_shared_from_this<ILevelRootGameObjects>
    {
    public:
        virtual ~ILevelRootGameObjects() = default;

        void AddRootGameObject(std::shared_ptr<IGameObject> rootGameObject)
        {
            root_game_objects_.push_back(rootGameObject);
        }

        void RemoveRootGameObject(std::shared_ptr<IGameObject> rootGameObject)
        {
            auto where = std::find_if(root_game_objects_.begin(),
                                      root_game_objects_.end(),
                                      [rootGameObject](std::shared_ptr<IGameObject> game_object)
                                      {
                                          return rootGameObject.get() == game_object.get();
                                      });

            if (where == root_game_objects_.end())
                return;
            
            root_game_objects_.erase(where);
        }

    protected:
        std::vector<std::shared_ptr<IGameObject>> root_game_objects_;
    };
}
