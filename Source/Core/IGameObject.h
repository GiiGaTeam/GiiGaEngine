#pragma once


#include<memory>
#include<json/json.h>

#include<Uuid.h>
#include<ITickable.h>


namespace GiiGa
{
    struct IComponent;

    struct IGameObject : public ITickable, public std::enable_shared_from_this<IGameObject>
    {
        virtual ~IGameObject() = default;

        virtual Uuid GetUuid() const =0;

        virtual Json::Value ToJsonWithComponents(bool is_prefab_root = false) const =0;

        virtual void RemoveComponent(std::shared_ptr<IComponent>) =0;

        virtual void Destroy() =0;

        virtual Uuid GetInPrefabUuid() const =0;
    };
}
