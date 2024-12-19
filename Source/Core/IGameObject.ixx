module;

#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>

export module IGameObject;

export import Uuid;
import ITickable;


namespace GiiGa
{
    export struct IGameObject : public ITickable, public std::enable_shared_from_this<IGameObject>
    {
        virtual ~IGameObject() = default;

        virtual Uuid GetUuid() const =0;

        virtual Json::Value ToJson() const =0;
    };
}
