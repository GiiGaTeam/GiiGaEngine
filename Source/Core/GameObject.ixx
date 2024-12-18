module;

#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <json/json.h>
#include <unordered_set>
#include <iostream>

export module GameObject;

export import IGameObject;
import IComponent;
import ITickable;
export import Uuid;
import TransformComponent;
import ConsoleComponent;
import Misc;
import IWorldQuery;
import ILevelRootGameObjects;

namespace GiiGa
{
    /*
     * LifeTime:
     *  of one GameObject is bound to parent
     *  cause it has shared_ptr to GameObject
     *  note: if GameObject does not has parent he should be in Levels root GameObjects list
     */
    export class GameObject final : public IGameObject
    {
    public:
        GameObject() = default;

        void Destroy()
        {
            TryRemoveFromLevelRoot();

            WorldQuery::GetInstance()->RemoveAnyWithUuid(uuid_);

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

        GameObject(const Uuid& uuid)
        {
            uuid_ = uuid;
        }

        /* GameObject Json
         * Name:
         * Uuid:
         * Parent:
         * Components:[...]
        */
        GameObject(const Json::Value& json, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr):
            level_root_gos_(level_rgo)
        {
            name_ = json["Name"].asString();

            auto js_uuid = Uuid::FromString(json["Uuid"].asString());

            if (!js_uuid.has_value())
            {
                throw std::runtime_error("Invalid UUID");
            }

            uuid_ = js_uuid.value();
        }

        void CreateComponents(const Json::Value& json)
        {
            for (auto&& comp_js : json["Components"])
            {
                if (comp_js["Type"].asString() == typeid(TransformComponent).name())
                {
                    CreateComponent<TransformComponent>(comp_js);
                }
                else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                {
                    CreateComponent<ConsoleComponent>(comp_js);
                }
            }
        }

        // does not register children
        void RegisterInWorld()
        {
            WorldQuery::AddAnyWithUuid(uuid_, std::static_pointer_cast<GameObject>(shared_from_this()));

            for (auto&& [_,component] : components_)
            {
                component->RegisterInWorld();
            }
        }

        void AttachToLevel(std::shared_ptr<ILevelRootGameObjects> level_rgo)
        {
            if (!level_root_gos_.expired())
                TryRemoveFromLevelRoot();

            level_root_gos_ = level_rgo;
            if (parent_.expired())
                level_rgo->AddRootGameObject(shared_from_this());

            for (auto&& [_,kid] : children_)
            {
                kid->AttachToLevel(level_rgo);
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
            AttachToLevel(level_root_gos_.lock());
        }

        Json::Value ToJson() const override
        {
            Json::Value result;

            Json::Value this_json;

            this_json["Name"] = name_;
            this_json["Uuid"] = uuid_.ToString();

            auto&& l_parent = parent_.lock();

            if (l_parent)
                this_json["Parent"] = l_parent->GetUuid().ToString();
            else
                this_json["Parent"] = Uuid::Null().ToString();

            for (auto&& [_,component] : components_)
            {
                this_json["Components"].append(component->ToJson());
            }

            result.append(this_json);

            for (auto&& [_,kid] : children_)
            {
                result.append(kid->ToJson());
            }

            return result;
        }

        void Restore(const Json::Value& json)
        {
            auto parentUuid = Uuid::FromString(json["Parent"].asString()).value();

            if (parentUuid != Uuid::Null())
                WorldQuery::GetWithUUID<std::shared_ptr<GameObject>>(parentUuid)->AddChild(std::static_pointer_cast<GameObject>(shared_from_this()));
            else
                AttachToLevel(level_root_gos_.lock());

            for (auto&& comp_js : json["Components"])
            {
                components_.at(Uuid::FromString(comp_js["Uuid"].asString()).value())->Restore(comp_js);
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
                newComp->SetOwner(shared_from_this());
                components_.insert({newComp->GetUuid(), newComp});
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

        void AddComponent(std::shared_ptr<IComponent> component) override
        {
            component->SetOwner(shared_from_this());
            components_.insert({component->GetUuid(), component});
        }

        void SetParent(std::shared_ptr<GameObject> parent, bool safeWorldTransfrom)
        {
            TryRemoveFromLevelRoot();
            parent->AddChild(std::static_pointer_cast<GameObject>(shared_from_this()));
            GetComponent<TransformComponent>()->AttachTo(parent->GetComponent<TransformComponent>());
        }

        virtual std::shared_ptr<GameObject> Clone()
        {
            auto newGameObject = std::make_shared<GameObject>();
            for (auto&& [_,component] : components_)
            {
                newGameObject->AddComponent(component->Clone());
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

        void AddChild(std::shared_ptr<GameObject> child)
        {
            children_.insert({child->GetUuid(), child});
            child->parent_ = std::static_pointer_cast<GameObject>(shared_from_this());
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
        std::string name_;
        std::weak_ptr<GameObject> parent_;
        std::unordered_map<Uuid, std::shared_ptr<IComponent>> components_;
        std::unordered_map<Uuid, std::shared_ptr<GameObject>> children_;
        std::weak_ptr<ILevelRootGameObjects> level_root_gos_;

        void TryRemoveFromLevelRoot()
        {
            auto l_level = level_root_gos_.lock();
            if (l_level && parent_.expired())
                l_level->RemoveRootGameObject(shared_from_this());
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
