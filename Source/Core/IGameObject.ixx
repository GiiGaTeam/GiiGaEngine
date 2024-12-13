module;

#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>

export module IGameObject;

import Uuid;

export class IComponent;

namespace GiiGa
{
    export struct IGameObject : public std::enable_shared_from_this<IGameObject>
    {
        virtual ~IGameObject() = default;

        virtual void OnUpdate() =0;

        virtual void AddComponent(const std::shared_ptr<IComponent>& newComp) =0;
    };
}
