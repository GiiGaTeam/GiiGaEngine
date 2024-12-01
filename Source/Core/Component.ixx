module;

#include <memory>
#include <json/json.h>

export module Component;

export import ITickable;
import Uuid;


namespace GiiGa
{
    export class GameObject;

    export class Component : public ITickable
    {
    public:
        virtual ~Component() = default;

        virtual void Init() = 0;

        virtual std::shared_ptr<Component> Clone() = 0;

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
        friend class GameObject;
        std::weak_ptr<GameObject> owner_;
    };
}  // namespace GiiGa
