export module IComponent;

import <memory>;
import <json/json.h>;
import <unordered_map>;

export import ITickable;
export import Uuid;
import IGameObject;

namespace GiiGa
{
    export struct IComponent : public ITickable, public std::enable_shared_from_this<IComponent>
    {
        virtual ~IComponent() override = default;
        
        virtual Json::Value ToJson() =0;

        virtual void RegisterInWorld()=0;

        virtual std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid) =0;
        
        virtual void Restore(const ::Json::Value&) =0;
        
        virtual void RestoreAsPrefab(const ::Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) =0;
        
        virtual void RestoreForClone(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) =0;

        virtual std::vector<Json::Value> GetModifications(std::shared_ptr<IComponent>) const = 0;
        
        virtual void Init() = 0;
        
        virtual void Destroy() =0;

        virtual Uuid GetUuid() const = 0;

        virtual Uuid GetInPrefabUuid() const = 0;

        virtual void SetOwner(std::shared_ptr<IGameObject> newOwner) = 0;
    };
}
