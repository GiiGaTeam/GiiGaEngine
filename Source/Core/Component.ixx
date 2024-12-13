module;

#include <memory>
#include <json/json.h>

export module Component;

export import ITickable;
import IComponent;
import Uuid;


namespace GiiGa
{
    export class GameObject;

    export class Component : public ITickable, public IComponent
    {
    public:
        virtual ~Component() = default;

        virtual void Init() = 0;

        virtual std::shared_ptr<IComponent> Clone() = 0;

        virtual void SetOwner(std::shared_ptr<GameObject> go)
        {
            owner_ = go;
        }

        virtual void Restore(const ::Json::Value&)=0;

        Uuid GetUuid() const
        {
            // TO DO
            Uuid::Null();
        }

        virtual Json::Value ToJSon() const
        {
            Json::Value result;
            //result["Owner"] = ;
            result["uuid"] = GetUuid().ToString();
            return result;
        }

    protected:
        std::weak_ptr<GameObject> owner_;
    };
} // namespace GiiGa
