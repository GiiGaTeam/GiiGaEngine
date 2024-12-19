module;

#include <memory>
#include <json/json.h>


export module IComponent;

export import ITickable;
export import Uuid;
import IGameObject;

namespace GiiGa
{
    export struct IComponent : public ITickable, public std::enable_shared_from_this<IComponent>
    {
        virtual ~IComponent() override = default;

        virtual std::shared_ptr<IComponent> Clone() =0;
        
        virtual Json::Value ToJson() =0;

        virtual void RegisterInWorld()=0;
        
        virtual void Restore(const ::Json::Value&) =0;
        
        virtual void Init() = 0;
        
        virtual void Destroy() =0;

        virtual Uuid GetUuid() const = 0;

        virtual void SetOwner(std::shared_ptr<IGameObject> newOwner) = 0;
    };
}
