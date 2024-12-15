module;

#include <memory>
#include <json/json.h>

export module Component;

export import ITickable;
import Uuid;


namespace GiiGa
{
    export class GameObject;

    export class Component : public ITickable, public std::enable_shared_from_this<Component>
    {
    public:
        virtual ~Component() = default;

        virtual void Init() = 0;

        virtual std::shared_ptr<Component> Clone() = 0;

        virtual void SetOwner(std::shared_ptr<GameObject> go)
        {
            owner_ = go;
        }

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
