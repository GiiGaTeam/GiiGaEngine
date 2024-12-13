module;

#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>

export module IGameObject;

import Uuid;
import ITickable;


namespace GiiGa
{
    export class IComponent;
    
    export struct IGameObject :  public ITickable, public std::enable_shared_from_this<IGameObject>
    {
        virtual ~IGameObject() = default;
        
        virtual void AddComponent(std::shared_ptr<IComponent> newComp) =0;
    };
}
