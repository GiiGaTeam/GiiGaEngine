#pragma once


#include<vector>
#include<memory>
#include<typeindex>
#include<unordered_map>
#include<unordered_set>
#include<optional>
#include<variant>

#include<IGameObject.h>
#include<IComponent.h>
#include<ITickable.h>
#include<Uuid.h>
#include<TransformComponent.h>
#include<Misc.h>
#include<IWorldQuery.h>
#include<ILevelRootGameObjects.h>
#include<PerObjectData.h>
#include<AssetHandle.h>
#include<PrefabInstanceModifications.h>

namespace GiiGa
{
    struct SpawnParameters
    {
        std::string name;
        std::weak_ptr<IGameObject> Owner;
        std::weak_ptr<ILevelRootGameObjects> LevelOverride;
    };

    /*
     * LifeTime:
     *  of one GameObject is bound to parent
     *  cause it has shared_ptr to GameObject
     *  note: if GameObject does not has parent he should be in Levels root GameObjects list
     */
    class GameObject final : public IGameObject
    {
        friend struct CreateComponentsForGameObject;

    public:
        std::string name = "GameObject";

        void Destroy() override
        {
            TryDetachFromParent();

            TryRemoveFromLevelRoot();
        }

        ~GameObject() override
        {
            //el::Loggers::getLogger("")->debug("GameObject::~GameObject");
            WorldQuery::RemoveAnyWithUuid(uuid_);
        }

        GameObject(const GameObject& other) = delete;
        GameObject(GameObject&& other) noexcept = default;
        GameObject& operator=(const GameObject& other) = delete;
        GameObject& operator=(GameObject&& other) noexcept = default;

        void BeginPlay()
        {
            for (auto& kid : children_)
            {
                kid->BeginPlay();
            }
            
            for (int i = 0; i < components_.size(); ++i)
            {
                WorldQuery::AddComponentToBeginPlayQueue(components_[i]);
            }
        }

        void EndPlay()
        {
            for (auto& kid : children_)
            {
                kid->EndPlay();
            }

            for (auto& component : components_)
            {
                component->EndPlay();
            }
        }

        void RemoveComponent(std::shared_ptr<IComponent> comp) override
        {
            components_.erase(std::remove(components_.begin(), components_.end(), comp));
        }

        void AttachToLevelRoot(std::shared_ptr<ILevelRootGameObjects> level_rgo)
        {
            if (!level_root_gos_.expired())
                TryRemoveFromLevelRoot();

            level_root_gos_ = level_rgo;
            if (parent_.expired())
                level_rgo->AddRootGameObject(shared_from_this());

            for (auto&& kid : children_)
            {
                kid->level_root_gos_ = level_rgo;
            }
        }

        void RecurSetLevel(std::shared_ptr<ILevelRootGameObjects> level_rgo)
        {
            this->level_root_gos_ = level_rgo;

            for (auto& kid : children_)
            {
                kid->RecurSetLevel(level_rgo);
            }
        }

        void TryDetachFromParent()
        {
            if (auto l_parent = parent_.lock())
            {
                l_parent->RemoveChild(std::static_pointer_cast<GameObject>(shared_from_this()));
                l_parent->GetComponent<TransformComponent>()->Detach();
            }
            parent_.reset();
            if (!level_root_gos_.expired())
                AttachToLevelRoot(level_root_gos_.lock());
        }

        static std::shared_ptr<GameObject> CreateEmptyGameObject(const SpawnParameters& spawnParameters)
        {
            auto newGameObject = std::shared_ptr<GameObject>(new GameObject());

            if (!spawnParameters.name.empty())
                newGameObject->name = spawnParameters.name;

            if (!newGameObject->GetComponent<TransformComponent>())
                newGameObject->transform_ = newGameObject->CreateComponent<TransformComponent>();

            if (!spawnParameters.Owner.expired())
            {
                newGameObject->AttachToLevelRoot(std::static_pointer_cast<GameObject>(spawnParameters.Owner.lock())->level_root_gos_.lock());
            }
            else
            {
                if (spawnParameters.LevelOverride.expired())
                    newGameObject->AttachToLevelRoot(WorldQuery::GetPersistentLevel());
                else
                    newGameObject->AttachToLevelRoot(spawnParameters.LevelOverride.lock());
            }

            newGameObject->RegisterInWorld();

            return newGameObject;
        }

        static std::shared_ptr<GameObject> CreateGameObjectFromJson(const Json::Value& json, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr, bool roll_id = false)
        {
            std::shared_ptr<GameObject> newGameObject = std::shared_ptr<GameObject>(new GameObject(json, level_rgo, roll_id));

            newGameObject->RegisterInWorld();

            return newGameObject;
        }

        std::unordered_map<Uuid, Uuid> GetInstanceIdMap(std::shared_ptr<GameObject> prefab_other)
        {
            std::unordered_map<Uuid, Uuid> InPrefabUuid_to_Instance;

            InPrefabUuid_to_Instance.insert({this->GetInPrefabUuid(), GetUuid()});

            const auto& prefab_components_prefab_uuids = prefab_other->GetComponentsByPrefabUuid();
            for (const auto& this_comp : this->components_)
            {
                if (prefab_components_prefab_uuids.contains(this_comp->GetInPrefabUuid()))
                {
                    InPrefabUuid_to_Instance.insert({this_comp->GetInPrefabUuid(), this_comp->GetUuid()});
                }
            }

            const auto& prefab_kids_prefab_uuids = prefab_other->GetKidsByPrefabUuid();
            for (const auto& this_kid : this->children_)
            {
                if (prefab_kids_prefab_uuids.contains(this_kid->GetInPrefabUuid()))
                {
                    const auto& kid_map = this_kid->GetInstanceIdMap(prefab_kids_prefab_uuids.at(this_kid->GetInPrefabUuid()));
                    InPrefabUuid_to_Instance.insert(kid_map.begin(), kid_map.end());
                }
            }

            return InPrefabUuid_to_Instance;
        }

        std::unordered_set<Uuid> GetRemovedGOsComps(std::shared_ptr<GameObject> prefab_other)
        {
            std::unordered_set<Uuid> Removed_GOs_Comps;

            const auto& this_components_prefab_uuids = this->GetComponentsByPrefabUuid();
            for (const auto& prefab_comp : prefab_other->components_)
            {
                if (!this_components_prefab_uuids.contains(prefab_comp->GetInPrefabUuid()))
                {
                    Removed_GOs_Comps.insert(prefab_comp->GetInPrefabUuid());
                }
            }

            const auto& this_kids_prefab_uuids = this->GetKidsByPrefabUuid();
            for (const auto& prefab_kid : prefab_other->children_)
            {
                if (!this_kids_prefab_uuids.contains(prefab_kid->GetInPrefabUuid()))
                {
                    Removed_GOs_Comps.insert(prefab_kid->GetInPrefabUuid());
                }
            }

            const auto& prefab_kids_prefab_uuids = prefab_other->GetKidsByPrefabUuid();
            for (auto&& kid : children_)
            {
                if (prefab_kids_prefab_uuids.contains(kid->GetInPrefabUuid()))
                {
                    const auto& kid_mods = kid->GetRemovedGOsComps(prefab_kids_prefab_uuids.at(kid->GetInPrefabUuid()));
                    Removed_GOs_Comps.insert(kid_mods.begin(), kid_mods.end());
                }
            }

            return Removed_GOs_Comps;
        }

        PrefabPropertyModifications GetPropertyModifications(std::shared_ptr<GameObject> prefab_other)
        {
            PrefabPropertyModifications modifications;

            if (this->name != prefab_other->name)
                modifications.insert({{this->inprefab_uuid_, "Name"}, this->name});

            if (!this->parent_.expired() && !prefab_other->parent_.expired())
            {
                if (this->parent_.lock()->GetInPrefabUuid() != prefab_other->parent_.lock()->GetInPrefabUuid())
                    modifications.insert({{this->inprefab_uuid_, "Parent"}, this->parent_.lock()->GetUuid().ToString()});
            }
            else
            {
                modifications.insert({{this->inprefab_uuid_, "Parent"}, this->parent_.lock()->GetUuid().ToString()});
            }

            const auto& prefab_components_prefab_uuids = prefab_other->GetComponentsByPrefabUuid();
            for (auto&& comp : components_)
            {
                if (prefab_components_prefab_uuids.contains(comp->GetInPrefabUuid()))
                {
                    std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> comp_mods = comp->GetPrefabInstanceModifications(prefab_components_prefab_uuids.at(comp->GetInPrefabUuid()));
                    modifications.insert(comp_mods.begin(), comp_mods.end());
                }
            }

            const auto& prefab_kids_prefab_uuids = prefab_other->GetKidsByPrefabUuid();
            for (auto&& kid : children_)
            {
                if (prefab_kids_prefab_uuids.contains(kid->inprefab_uuid_))
                {
                    const auto& kid_mods = kid->GetPropertyModifications(prefab_kids_prefab_uuids.at(kid->inprefab_uuid_));
                    modifications.insert(kid_mods.begin(), kid_mods.end());
                }
            }

            return modifications;
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications)
        {
            if (modifications.contains({this->GetInPrefabUuid(), "Name"}))
            {
                auto new_val_js = modifications.at({this->GetInPrefabUuid(), "Name"});
                auto new_name = new_val_js.asString();
                this->name = new_name;
            }

            if (modifications.contains({this->inprefab_uuid_, "Parent"}))
            {
                Uuid parentUuid = Uuid::FromString(modifications.at({this->inprefab_uuid_, "Parent"}).asString()).value();

                if (parentUuid == Uuid::Null())
                {
                    TryDetachFromParent();
                }
                else
                {
                    SetParent(WorldQuery::GetWithUUID<GameObject>(parentUuid));
                }
            }

            for (auto& comp : components_)
            {
                comp->ApplyModifications(modifications);
            }

            for (auto& kid : children_)
            {
                kid->ApplyModifications(modifications);
            }
        }

        std::unordered_map<Uuid, std::shared_ptr<GameObject>> GetKidsByPrefabUuid()
        {
            std::unordered_map<Uuid, std::shared_ptr<GameObject>> result;
            for (auto&& kid : children_)
            {
                if (kid->GetInPrefabUuid() != Uuid::Null())
                    result.emplace(std::pair{kid->inprefab_uuid_, kid});
            }

            return result;
        }

        std::unordered_map<Uuid, std::shared_ptr<IComponent>> GetComponentsByPrefabUuid()
        {
            std::unordered_map<Uuid, std::shared_ptr<IComponent>> result;
            for (auto&& comp : components_)
            {
                if (comp->GetInPrefabUuid() != Uuid::Null())
                    result.emplace(std::pair{comp->GetInPrefabUuid(), comp});
            }

            return result;
        }

        std::unordered_map<Uuid, std::shared_ptr<GameObject>> GetKidsByWorldUuid()
        {
            std::unordered_map<Uuid, std::shared_ptr<GameObject>> result;
            for (auto&& kid : children_)
            {
                result.emplace(std::pair{kid->GetUuid(), kid});
            }

            return result;
        }

        std::unordered_map<Uuid, std::shared_ptr<IComponent>> GetComponentsByWorldUuid()
        {
            std::unordered_map<Uuid, std::shared_ptr<IComponent>> result;
            for (auto&& comp : components_)
            {
                result.emplace(std::pair{comp->GetUuid(), comp});
            }

            return result;
        }

        Json::Value ToJsonWithComponents(bool is_prefab_root = false) const override
        {
            Json::Value this_json;

            this_json["Name"] = name;
            this_json["Uuid"] = uuid_.ToString();

            if (!parent_.expired() && !is_prefab_root)
                this_json["Parent"] = parent_.lock()->GetUuid().ToString();
            else
                this_json["Parent"] = Uuid::Null().ToString();

            for (auto&& component : components_)
            {
                this_json["Components"].append(component->ToJson(is_prefab_root));
            }

            return this_json;
        }

        void RestoreFromLevelJson(const Json::Value& json)
        {
            auto parent_uuid = Uuid::FromString(json["Parent"].asString()).value();

            if (parent_uuid != Uuid::Null())
            {
                SetParent(WorldQuery::GetWithUUID<GameObject>(parent_uuid));
            }

            const auto& comps_by_uuid = GetComponentsByWorldUuid();
            for (auto&& comp_js : json["Components"])
            {
                std::string comp_id_str = comp_js["Uuid"].asString();
                Uuid comp_id = Uuid::FromString(comp_id_str).value();
                comps_by_uuid.at(comp_id)->RestoreFromLevelJson(comp_js);
            }
        }

        void RestoreAsPrefab(const Json::Value& go_js, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world)
        {
            auto parent_uuid_prefab = Uuid::FromString(go_js["Parent"].asString()).value();

            if (parent_uuid_prefab != Uuid::Null())
            {
                auto world_parent = prefab_uuid_to_world.at(parent_uuid_prefab);
                SetParent(WorldQuery::GetWithUUID<GameObject>(world_parent));
            }

            const auto& comps_by_uuid = GetComponentsByWorldUuid();

            for (auto&& comp_js : go_js["Components"])
            {
                std::string comp_prefab_id_str = comp_js["Uuid"].asString();
                Uuid comp_prefab_id = Uuid::FromString(comp_prefab_id_str).value();
                Uuid comp_world_id = prefab_uuid_to_world.at(comp_prefab_id);
                comps_by_uuid.at(comp_world_id)->RestoreAsPrefab(comp_js, prefab_uuid_to_world);
            }
        }

        void RestoreFromOriginal(std::shared_ptr<GameObject> original, const std::unordered_map<Uuid, Uuid>& original_uuid_to_world)
        {
            for (int i = 0; i < original->children_.size(); ++i)
            {
                this->children_[i]->RestoreFromOriginal(original->children_[i], original_uuid_to_world);
            }

            for (int i = 0; i < original->components_.size(); ++i)
            {
                this->components_[i]->RestoreFromOriginal(original->components_[i], original_uuid_to_world);
            }
        }

        void Tick(float dt) override
        {
            for (int i = 0; i < components_.size(); ++i)
                components_[i]->Tick(dt);

            for (int i = 0; i < children_.size(); ++i)
                children_[i]->Tick(dt);
        }

        template <typename T, typename... Args>
        std::shared_ptr<T> CreateComponent(const Args&... args)
        {
            if (std::shared_ptr<T> newComp = std::make_shared<T>(args...))
            {
                newComp->SetOwner(std::static_pointer_cast<GameObject>(shared_from_this()));
                components_.push_back(newComp);
                newComp->RegisterInWorld();
                return newComp;
            }
            return nullptr;
        }

        // note: do not forget to register component in world
        void AddComponent(std::shared_ptr<IComponent> comp)
        {
            comp->SetOwner(shared_from_this());
            components_.push_back(comp);
        }

        template <typename T>
        std::shared_ptr<T> GetComponent()
        {
            for (auto&& component : components_)
            {
                if (std::shared_ptr<T> comp = std::dynamic_pointer_cast<T>(component))
                    return comp;
            }
            return nullptr;
        }

        std::weak_ptr<TransformComponent> GetTransformComponent()
        {
            return transform_;
        }

        void SetParent(std::shared_ptr<GameObject> parent, bool safeWorldTransfrom = false)
        {
            TryRemoveFromLevelRoot();
            const auto& kids_by_world_uuid = parent->GetKidsByWorldUuid();
            if (!kids_by_world_uuid.contains(this->uuid_))
            {
                parent->children_.push_back(std::static_pointer_cast<GameObject>(shared_from_this()));
                this->parent_ = parent;
            }

            if (this->level_root_gos_.lock() != parent->level_root_gos_.lock())
            {
                this->RecurSetLevel(parent->level_root_gos_.lock());
            }

            GetComponent<TransformComponent>()->AttachTo(parent->GetComponent<TransformComponent>());
        }

        std::shared_ptr<GameObject> Clone(
            std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid,
            const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid,
            const std::optional<std::unordered_set<Uuid>>& removed_gos_comps)
        {
            auto newGameObject = std::shared_ptr<GameObject>(new GameObject());

            if (instance_uuid)
                newGameObject->uuid_ = instance_uuid.value().at(this->GetInPrefabUuid());

            prefab_uuid_to_world_uuid[this->GetUuid()] = newGameObject->GetUuid();
            newGameObject->RegisterInWorld();

            newGameObject->prefab_handle_ = this->prefab_handle_;
            newGameObject->inprefab_uuid_ = this->inprefab_uuid_;

            newGameObject->name = this->name;

            for (auto&& component : components_)
            {
                if (!removed_gos_comps.has_value() || !removed_gos_comps.value().contains(component->GetInPrefabUuid()))
                    newGameObject->AddComponent(component->Clone(prefab_uuid_to_world_uuid, instance_uuid));
            }

            if (auto opt_trans_comp = newGameObject->GetComponent<TransformComponent>())
                newGameObject->transform_ = opt_trans_comp;
            else
                throw std::exception("Failed to find transform component");

            for (auto&& kid : children_)
            {
                if (!removed_gos_comps.has_value() || !removed_gos_comps.value().contains(kid->GetInPrefabUuid()))
                {
                    auto kid_clone = kid->Clone(prefab_uuid_to_world_uuid, instance_uuid, removed_gos_comps);
                    kid_clone->SetParent(newGameObject);
                }
            }

            return newGameObject;
        }

        const auto& GetComponents()
        {
            return components_;
        }

        const auto& GetChildren() const
        {
            return children_;
        }

        size_t GetChildrenCount() const
        {
            return children_.size();
        }

        void AddChild(std::shared_ptr<GameObject> child)
        {
            child->SetParent(std::static_pointer_cast<GameObject>(shared_from_this()));
        }

        void RemoveChild(std::shared_ptr<GameObject> child)
        {
            children_.erase(std::find(children_.begin(), children_.end(), child));
        }

        Uuid GetUuid() const override
        {
            return uuid_;
        }

        Uuid GetInPrefabUuid() const override
        {
            return inprefab_uuid_;
        }

        std::shared_ptr<GameObject> GetParent()
        {
            return parent_.lock();
        }

        AssetHandle prefab_handle_;

    protected:
        void OnBeginOverlap(const std::shared_ptr<CollisionComponent>& other_comp, const CollideInfo& collideInfo) override
        {
            for (auto& component : components_)
            {
                component->OnBeginOverlap(other_comp, collideInfo);
            }
        }

        void OnOverlapping(const std::shared_ptr<CollisionComponent>& other_comp, const CollideInfo& collideInfo) override
        {
            for (auto& component : components_)
            {
                component->OnOverlapping(other_comp, collideInfo);
            }
        }

        void OnEndOverlap(const std::shared_ptr<CollisionComponent>& other_comp) override
        {
            for (auto& component : components_)
            {
                component->OnEndOverlap(other_comp);
            }
        }

    private:
        Uuid uuid_ = Uuid::New();
        Uuid inprefab_uuid_ = Uuid::Null();
        std::weak_ptr<GameObject> parent_;
        std::vector<std::shared_ptr<IComponent>> components_;
        std::vector<std::shared_ptr<GameObject>> children_;
        std::weak_ptr<ILevelRootGameObjects> level_root_gos_;

        std::shared_ptr<TransformComponent> transform_;

        std::shared_ptr<PerObjectData> perObjectData_;

        void TryRemoveFromLevelRoot()
        {
            auto l_level = level_root_gos_.lock();
            if (l_level && parent_.expired())
                l_level->RemoveRootGameObject(shared_from_this());
        }

        GameObject() = default;

        /* GameObject Json
         * Name:
         * Uuid:
         * Components:[...]
         * Parent: UUID
        */
        GameObject(const Json::Value& json, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr, bool roll_id = false):
            level_root_gos_(level_rgo)
        {
            name = json["Name"].asString();

            auto js_uuid = Uuid::FromString(json["Uuid"].asString());

            if (!js_uuid.has_value())
            {
                throw std::runtime_error("Invalid UUID");
            }

            if (!roll_id)
                uuid_ = js_uuid.value();
            else
            {
                inprefab_uuid_ = js_uuid.value();
                uuid_ = Uuid::New();
            }
        }

        // does not register children
        void RegisterInWorld()
        {
            WorldQuery::AddAnyWithUuid(uuid_, shared_from_this());
        }

        void CreateChildren(const Json::Value& json, bool roll_id = false)
        {
            for (auto&& kid_js : json["Children"])
            {
                auto kid_go = CreateGameObjectFromJson(kid_js, this->level_root_gos_.lock(), roll_id);
                this->AddChild(kid_go);
            }
        }
    };
} // namespace GiiGa

template <>
struct std::hash<GiiGa::GameObject>
{
    std::size_t operator()(const GiiGa::GameObject& obj) const
    {
        // Assuming Uuid class has an appropriate std::hash specialization or 
        // a method to convert to a hashable format
        return obj.GetUuid().Hash();
    }
};
