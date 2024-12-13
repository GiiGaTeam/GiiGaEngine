module;

#include <memory>
#include <json/json.h>

export module IComponent;

export import ITickable;
export class IGameObject;

namespace GiiGa
{
    export struct IComponent : public ITickable, public std::enable_shared_from_this<IComponent>
    {
        virtual ~IComponent() = default;

        virtual void OnStart() = 0;
        virtual void OnUpdate() = 0;

        virtual std::shared_ptr<IComponent> Clone() =0;
        virtual Json::Value ToJson() =0;

        virtual void SetOwner(std::shared_ptr<IGameObject> newOwner) = 0;
    };
}
