#pragma once


#include<memory>
#include<json/json.h>
#include<unordered_map>
#include<optional>

#include<ITickable.h>
#include<Uuid.h>
#include<IGameObject.h>
#include<PrefabInstanceModifications.h>

namespace GiiGa
{
    struct IComponent : public ITickable, public std::enable_shared_from_this<IComponent>
    {
        virtual ~IComponent() override = default;

        virtual Json::Value ToJson(bool is_prefab_root = false) =0;

        virtual void RegisterInWorld() =0;

        virtual std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>
                                                  & instance_uuid) =0;

        virtual void Restore(const ::Json::Value&) =0;

        virtual void RestoreAsPrefab(const ::Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) =0;

        virtual void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) =0;

        virtual std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const = 0;

        virtual void ApplyModifications(const PrefabPropertyModifications& modifications) =0;

        virtual void Init() = 0;

        virtual void BeginPlay() = 0;

        virtual void Destroy() =0;

        virtual Uuid GetUuid() const = 0;

        virtual Uuid GetInPrefabUuid() const = 0;

        virtual void SetOwner(std::shared_ptr<IGameObject> newOwner) = 0;
    };
}
