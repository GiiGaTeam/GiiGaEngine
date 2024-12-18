module;

#include <memory>
#include <json/json.h>
#include <stduuid/uuid.h>

export module Component;

export import ITickable;
import IComponent;
import Uuid;
import IWorldQuery;

namespace GiiGa
{
    /*
     * Each Component Should register it self in WorldQuery Singleton
     * upon creation, and unregister on destroy
     */
    export class Component : public IComponent
    {
    public:
        Component() = default;

        Component(Json::Value json)
        {
            auto js_uuid = Uuid::FromString(json["Uuid"].asString());

            if (!js_uuid.has_value())
            {
                throw std::runtime_error("Invalid UUID");
            }

            uuid_ = js_uuid.value();
        }

        void RegisterInWorld() override
        {
            WorldQuery::AddComponent(shared_from_this());
            WorldQuery::AddAnyWithUuid(uuid_, std::static_pointer_cast<Component>(shared_from_this()));
        }

        void Destroy() override
        {
            WorldQuery::RemoveComponent(shared_from_this());
            WorldQuery::RemoveAnyWithUuid(uuid_);
        }

        virtual ~Component() override = default;

        virtual void Init() = 0;

        virtual std::shared_ptr<IComponent> Clone() = 0;

        virtual void SetOwner(std::shared_ptr<IGameObject> go) override
        {
            owner_ = go;
        }

        Uuid GetUuid() const override
        {
            return uuid_;
        }

        virtual Json::Value ToJson() override
        {
            Json::Value result;
            result["Uuid"] = GetUuid().ToString();
            return result;
        }

        void Tick(float dt) override =0;

    protected:
        Uuid uuid_ = Uuid::New();
        bool enabled = true;
        std::weak_ptr<IGameObject> owner_;
    };
} // namespace GiiGa
