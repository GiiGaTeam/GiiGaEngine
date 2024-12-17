module;

#include <memory>
#include <json/json.h>
#include <stduuid/uuid.h>

export module Component;

export import ITickable;
import IComponent;
import Uuid;
import IComponentsInLevel;

namespace GiiGa
{
    export class Component : public IComponent
    {
    public:
        Component(std::shared_ptr<IComponentsInLevel> inLevel = nullptr):
            componentsInLevel_(inLevel)
        {
            if (inLevel)
                inLevel->AddComponent(shared_from_this());
            else
                enabled = false;
        }

        Component(Uuid uuid, std::shared_ptr<IComponentsInLevel> inLevel = nullptr):
            Component(inLevel)
        {
            uuid_ = uuid;
        }

        void Destroy() override
        {
            if (auto l_level = componentsInLevel_.lock())
                l_level->RemoveComponent(shared_from_this());
        }

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
        bool enabled = true;
        std::weak_ptr<IGameObject> owner_;
        std::weak_ptr<IComponentsInLevel> componentsInLevel_;
    };
} // namespace GiiGa
