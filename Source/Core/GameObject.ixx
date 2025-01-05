export module GameObject;

import <vector>;
import <memory>;
import <typeindex>;
import <unordered_map>;
import <json/json.h>;
import <unordered_set>;
import <iostream>;
import <optional>;

export import IGameObject;
import IComponent;
import ITickable;
export import Uuid;
import TransformComponent;
import ConsoleComponent;
import Misc;
import IWorldQuery;
import ILevelRootGameObjects;
import PerObjectData;
import AssetHandle;
import Logger;

namespace GiiGa
{
    export struct SpawnParameters
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
    export class GameObject final : public IGameObject
    {
        friend struct CreateComponentsForGameObject;

    public:
        std::string name = "GameObject";

        void Destroy() override
        {
            if (!pending_to_destroy_)
            {
                WorldQuery::EmplaceGOToDestroy(shared_from_this());
                pending_to_destroy_ = true;
                return;
            }

            DetachFromParent();

            TryRemoveFromLevelRoot();

            WorldQuery::RemoveAnyWithUuid(uuid_);

            for (auto&& kid : children_)
            {
                kid->Destroy();
            }

            for (auto&& component : components_)
            {
                component->Destroy();
            }
        }

        ~GameObject() override = default;

        GameObject(const GameObject& other) = delete;
        GameObject(GameObject&& other) noexcept = default;
        GameObject& operator=(const GameObject& other) = delete;
        GameObject& operator=(GameObject&& other) noexcept = default;

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

        void DetachFromParent()
        {
            if (auto l_parent = parent_.lock())
            {
                l_parent->RemoveChild(std::static_pointer_cast<GameObject>(shared_from_this()));
                l_parent->GetComponent<TransformComponent>()->Detach();
            }
            parent_.reset();
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

        std::vector<Json::Value> FindModifications(std::shared_ptr<GameObject> prefab_other)
        {
            std::vector<Json::Value> result;

            std::unordered_map<Uuid, Uuid> kids_prefab_uuid_to_current;
            std::unordered_map<Uuid, Uuid> components_prefab_uuid_to_current;

            const auto& prefab_kids_uuids = prefab_other->GetKidsByPrefabUuid();
            const auto& prefab_components_uuids = prefab_other->GetComponentsByPrefabUuid();

            for (auto&& comp : components_)
            {
                if (!prefab_components_uuids.contains(comp->GetInPrefabUuid()))
                {
                    Json::Value modification;
                    modification["Target"] = comp->GetInPrefabUuid().ToString();
                    modification["Modification"] = "Remove";
                    result.push_back(modification);
                }
                else
                {
                    auto comp_mods = comp->GetModifications(prefab_components_uuids.at(comp->GetInPrefabUuid()));
                    result.insert(result.end(), comp_mods.begin(), comp_mods.end());
                }
            }

            for (auto&& kid : children_)
            {
                if (!prefab_kids_uuids.contains(kid->inprefab_uuid_))
                {
                    Json::Value modification;
                    modification["Target"] = kid->inprefab_uuid_.ToString();
                    modification["Modification"] = "Remove";
                    result.push_back(modification);
                }
                else
                {
                    auto kid_modifications = kid->FindModifications(prefab_kids_uuids.at(kid->inprefab_uuid_));
                    result.insert(result.end(), kid_modifications.begin(), kid_modifications.end());
                }
            }

            return result;
        }

        std::unordered_map<Uuid, std::shared_ptr<GameObject>> GetKidsByPrefabUuid()
        {
            std::unordered_map<Uuid, std::shared_ptr<GameObject>> result;
            for (auto&& kid : children_)
            {
                result.emplace(std::pair{kid->inprefab_uuid_, kid});
            }

            return result;
        }

        std::unordered_map<Uuid, std::shared_ptr<IComponent>> GetComponentsByPrefabUuid()
        {
            std::unordered_map<Uuid, std::shared_ptr<IComponent>> result;
            for (auto&& comp : components_)
            {
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

        void Restore(const Json::Value& json)
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
                comps_by_uuid.at(comp_id)->Restore(comp_js);
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
                parent->AddChild(std::static_pointer_cast<GameObject>(shared_from_this()));
                parent_ = parent;
            }

            GetComponent<TransformComponent>()->AttachTo(parent->GetComponent<TransformComponent>());
        }

        std::shared_ptr<GameObject> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid)
        {
            auto newGameObject = std::shared_ptr<GameObject>(new GameObject());

            newGameObject->name = this->name;
            newGameObject->inprefab_uuid_ = this->inprefab_uuid_;

            el::Loggers::getLogger(LogWorld)->debug("Clone: Add key: %v, value: %v to prefab_uuid_to_world_uuid",
                                                    this->GetUuid().ToString(), newGameObject->GetUuid().ToString());

            original_uuid_to_world_uuid[this->GetUuid()] = newGameObject->GetUuid();

            newGameObject->RegisterInWorld();

            for (auto&& component : components_)
            {
                newGameObject->AddComponent(component->Clone(original_uuid_to_world_uuid));
            }

            auto trans_comp_it = std::find_if(newGameObject->components_.begin(), newGameObject->components_.end(),
                                              [](std::shared_ptr<IComponent> comp)
                                              {
                                                  return typeid(*comp).hash_code() == typeid(TransformComponent).hash_code();
                                              });

            if (trans_comp_it != newGameObject->components_.end())
                newGameObject->transform_ = std::dynamic_pointer_cast<TransformComponent>(*trans_comp_it);
            else
                throw std::exception("Failed to find transform component");

            for (auto&& kid : children_)
            {
                auto kid_clone = kid->Clone(original_uuid_to_world_uuid);
                kid_clone->SetParent(newGameObject);
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
            children_.push_back(child);
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

        Uuid GetInprefabUuid() const
        {
            return inprefab_uuid_;
        }

        AssetHandle GetPrefabHandle()
        {
            return prefab_handle_;
        }

        std::shared_ptr<GameObject> GetParent()
        {
            return parent_.lock();
        }

    private:
        Uuid uuid_ = Uuid::New();
        Uuid inprefab_uuid_ = Uuid::Null();
        AssetHandle prefab_handle_;
        std::weak_ptr<GameObject> parent_;
        std::vector<std::shared_ptr<IComponent>> components_;
        std::vector<std::shared_ptr<GameObject>> children_;
        std::weak_ptr<ILevelRootGameObjects> level_root_gos_;

        std::shared_ptr<TransformComponent> transform_;

        std::shared_ptr<PerObjectData> perObjectData_;

        bool pending_to_destroy_ = false;

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
            WorldQuery::AddAnyWithUuid(uuid_, std::static_pointer_cast<GameObject>(shared_from_this()));
        }

        void CreateChildren(const Json::Value& json, bool roll_id = false)
        {
            for (auto&& kid_js : json["Children"])
            {
                auto kid_go = CreateGameObjectFromJson(kid_js, this->level_root_gos_.lock(), roll_id);
                this->AddChild(kid_go);
            }
        }

        void AddComponent(std::shared_ptr<IComponent> newComp)
        {
            newComp->SetOwner(std::static_pointer_cast<GameObject>(shared_from_this()));
            components_.push_back(newComp);
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
