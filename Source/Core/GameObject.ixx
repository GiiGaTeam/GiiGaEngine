export module GameObject;

import <vector>;
import <memory>;
import <typeindex>;
import <unordered_map>;
import <json/json.h>;
import <unordered_set>;
import <iostream>;
import <directxtk12/SimpleMath.h>;

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
        friend void CreateComponentsForGameObject(std::shared_ptr<GameObject> gameObject, const Json::Value& go_js, bool roll_id);

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

            for (auto&& [_,kid] : children_)
            {
                kid->Destroy();
            }

            for (auto&& [_,component] : components_)
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

            for (auto&& [_,kid] : children_)
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

        Json::Value ToJson() const
        {
            Json::Value this_json;

            this_json["Name"] = name;
            this_json["Uuid"] = uuid_.ToString();

            if (!parent_.expired())
                this_json["Parent"] = parent_.lock()->GetUuid().ToString();
            else
                this_json["Parent"] = Uuid::Null().ToString();

            for (auto&& [_,component] : components_)
            {
                this_json["Components"].append(component->ToJson());
            }

            return this_json;
        }

        std::vector<Json::Value> ToJsonWithKids() const override
        {
            std::vector<Json::Value> jsons;

            jsons.push_back(ToJson());

            for (auto&& [_,kid] : children_)
            {
                jsons.push_back(kid->ToJson());
            }

            return jsons;
        }

        void Restore(const Json::Value& json)
        {
            auto parent_uuid = Uuid::FromString(json["Parent"].asString()).value();

            if (parent_uuid != Uuid::Null())
            {
                SetParent(WorldQuery::GetWithUUID<GameObject>(parent_uuid));
            }

            for (auto&& comp_js : json["Components"])
            {
                std::string comp_id_str = comp_js["Uuid"].asString();
                Uuid comp_id = Uuid::FromString(comp_id_str).value();
                components_.at(comp_id)->Restore(comp_js);
            }
        }

        void Tick(float dt) override
        {
            for (auto&& [_,component] : components_)
            {
                component->Tick(dt);
            }

            for (auto&& [_,kid] : children_)
            {
                kid->Tick(dt);
            }
        }

        template <typename T, typename... Args>
        std::shared_ptr<T> CreateComponent(const Args&... args)
        {
            if (std::shared_ptr<T> newComp = std::make_shared<T>(args...))
            {
                newComp->SetOwner(std::static_pointer_cast<GameObject>(shared_from_this()));
                components_.insert({newComp->GetUuid(), newComp});
                newComp->RegisterInWorld();
                return newComp;
            }
            return nullptr;
        }

        template <typename T>
        std::shared_ptr<T> GetComponent()
        {
            for (auto&& [_,component] : components_)
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

            if (!parent->children_.contains(this->uuid_))
            {
                parent->AddChild(std::static_pointer_cast<GameObject>(shared_from_this()));
                parent_ = parent;
            }

            GetComponent<TransformComponent>()->AttachTo(parent->GetComponent<TransformComponent>());
        }

        virtual std::shared_ptr<GameObject> Clone()
        {
            Todo();
            auto newGameObject = std::shared_ptr<GameObject>(new GameObject());
            for (auto&& [_,component] : components_)
            {
                //newGameObject->AddComponent(component->Clone());
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
            children_.insert({child->GetUuid(), child});
            child->SetParent(std::static_pointer_cast<GameObject>(shared_from_this()));
        }

        void RemoveChild(std::shared_ptr<GameObject> child)
        {
            children_.erase(child->GetUuid());
        }

        Uuid GetUuid() const override
        {
            return uuid_;
        }

        std::shared_ptr<GameObject> GetParent()
        {
            return parent_.lock();
        }

    private:
        Uuid uuid_ = Uuid::New();
        std::weak_ptr<GameObject> parent_;
        std::unordered_map<Uuid, std::shared_ptr<IComponent>> components_;
        std::unordered_map<Uuid, std::shared_ptr<GameObject>> children_;
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
                uuid_ = Uuid::New();
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
