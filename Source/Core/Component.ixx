export module Component;

import <memory>;
import <json/json.h>;
import <stduuid/uuid.h>;

export import ITickable;
import IComponent;
import IGameObject;
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

        Component(const Json::Value& json, bool roll_id = false)
        {
            auto js_uuid = Uuid::FromString(json["Uuid"].asString());

            if (!js_uuid.has_value())
            {
                throw std::runtime_error("Invalid UUID");
            }

            if (!roll_id)
                uuid_ = js_uuid.value();
            else
                uuid_ = Uuid::New();
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
            Json::Value json;

            json["Uuid"] = uuid_.ToString();

            // Combining json with the result of DerivedToJson
            Json::Value derivedJson = DerivedToJson();
            for (Json::Value::const_iterator it = derivedJson.begin(); it != derivedJson.end(); ++it)
            {
                json[it.key().asString()] = *it;
            }

            return json;
        }


        virtual Json::Value DerivedToJson() =0;

        void Tick(float dt) override =0;

    protected:
        Uuid uuid_ = Uuid::New();
        bool enabled = true;
        std::weak_ptr<IGameObject> owner_;
    };
} // namespace GiiGa
