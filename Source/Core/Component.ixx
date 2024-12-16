module;

#include <memory>
#include <json/json.h>
#include <stduuid/uuid.h>

export module Component;

export import ITickable;
import IComponent;
import Uuid;


namespace GiiGa
{
    export class Component : public IComponent
    {
    public:
        virtual ~Component() = default;

        virtual void Init() = 0;

        virtual std::shared_ptr<IComponent> Clone() = 0;

        virtual void SetOwner(std::shared_ptr<IGameObject> go) override
        {
            owner_ = go;
        }

        virtual void Restore(const ::Json::Value&) =0;

        Uuid GetUuid() const
        {
            return uuid_;
        }

        virtual Json::Value ToJson() override
        {
            Json::Value result;
            //result["Owner"] = ;
            result["uuid"] = GetUuid().ToString();
            return result;
        }

        void Tick(float dt) override =0;

    protected:
        Uuid uuid_ = Uuid::New();
        std::weak_ptr<IGameObject> owner_;
    };
} // namespace GiiGa
