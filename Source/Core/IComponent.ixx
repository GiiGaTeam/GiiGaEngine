module;

#include <memory>
#include <json/json.h>

export module IComponent;

export import ITickable;

namespace GiiGa
{
    export class IGameObject;
    
    export struct IComponent : public ITickable, public std::enable_shared_from_this<IComponent>
    {
        virtual ~IComponent() = default;
        
        virtual std::shared_ptr<IComponent> Clone() =0;
        virtual Json::Value ToJson() =0;

        virtual void Destroy()=0;

        virtual void SetOwner(std::shared_ptr<IGameObject> newOwner) = 0;
    };
}
