module;

#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <json/json.h>
#include <unordered_set>
#include <iostream>
#include <directxtk12/SimpleMath.h>

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

        static std::shared_ptr<GameObject> CreateEmptyGameObjectInLevelRoot(std::shared_ptr<ILevelRootGameObjects> level_rgo)
        {
            auto gameObject = std::shared_ptr<GameObject>(new GameObject());
            gameObject->AttachToLevel(level_rgo);
            gameObject->RegisterInWorld();

            return gameObject;
        }

        static std::shared_ptr<GameObject> CreateGameObjectFromJson(const Json::Value& json, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr)
        {
            std::shared_ptr<GameObject> newGameObject = std::shared_ptr<GameObject>(new GameObject(json, level_rgo));

            newGameObject->RegisterInWorld();

            newGameObject->CreateComponents(json);

            return newGameObject;
        }

        Json::Value ToJson() const override
        {
            Json::Value this_json;

            this_json["Name"] = name_;
            this_json["Uuid"] = uuid_.ToString();
            
            for (auto&& [_,component] : components_)
            {
                this_json["Components"].append(component->ToJson());
            }

            for (auto&& [_, kid] : children_)
            {
                this_json["Children"].append(kid->ToJson());
            }

            return this_json;
        }

        void Restore(const Json::Value& json)
        {
            for (auto&& comp_js : json["Components"])
            {
                components_.at(Uuid::FromString(comp_js["Uuid"].asString()).value())->Restore(comp_js);
            }

            for (auto&& kid_js : json["Children"])
            {
                children_.at(Uuid::FromString(kid_js["Uuid"].asString()).value())->Restore(kid_js);
            }
        }

        void Init()
        {
            if (transform_.expired()) transform_ = CreateComponent<TransformComponent>();

            for (auto&& [_,component] : components_)
            {
                component->Init();
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

            if (parent->children_.find(this->uuid_) == parent->children_.end())
                parent->AddChild(std::static_pointer_cast<GameObject>(shared_from_this()));

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
        std::string name_;
        std::weak_ptr<GameObject> parent_;
        std::unordered_map<Uuid, std::shared_ptr<IComponent>> components_;
        std::unordered_map<Uuid, std::shared_ptr<GameObject>> children_;
        std::weak_ptr<ILevelRootGameObjects> level_root_gos_;

        std::weak_ptr<TransformComponent> transform_;

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
         * Children: [...]
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

        // does not register children
        void RegisterInWorld()
        {
            WorldQuery::AddAnyWithUuid(uuid_, std::static_pointer_cast<GameObject>(shared_from_this()));
        }

        void CreateComponents(const Json::Value& json)
        {
            for (auto&& comp_js : json["Components"])
            {
                if (comp_js["Type"].asString() == typeid(TransformComponent).name())
                {
                    transform_ = CreateComponent<TransformComponent>(comp_js);
                }
                else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                {
                    CreateComponent<ConsoleComponent>(comp_js);
                }
            }
        }

        void CreateChildren(const Json::Value& json)
        {
            for (auto&& kid_js : json["Children"])
            {
                auto kid_go = CreateGameObjectFromJson(kid_js, this->level_root_gos_.lock());
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
