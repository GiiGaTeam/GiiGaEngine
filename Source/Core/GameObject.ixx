module;

#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <json/json.h>
#include <unordered_set>

export module GameObject;

export import IGameObject;
import IComponent;
import ITickable;
import Uuid;
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
     * Prefab vs GameObject:
     *  Prefab has no level_root_gos_ but is_in_root
     *  GameObject if is_in_root it has level_root_gos_
     *      if it is not is_in_root it has no level_root_gos_
     */
    export class GameObject final : public IGameObject
    {
    public:
        GameObject(std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr):
            level_root_gos_(level_rgo)
        {
            if (level_rgo)
                level_rgo->AddRootGameObject(shared_from_this());

            WorldQuery::GetInstance()->AddAnyWithUuid(uuid_, std::static_pointer_cast<GameObject>(shared_from_this()));
        }

        void Destroy()
        {
            if (auto l_level = level_root_gos_.lock())
                l_level->RemoveRootGameObject(shared_from_this());

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

        GameObject(const Uuid& uuid, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr):
            GameObject(level_rgo)
        {
            uuid_ = uuid;
        }

        /* GameObject Json
         * Name:
         * Uuid:
         * Components:[...]
         * Children:[...]
        */
        GameObject(Json::Value json, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr):
            GameObject(level_rgo)
        {
            name_ = json["Name"].asString();

            auto js_uuid = Uuid::FromString(json["Uuid"].asString());

            if (!js_uuid.has_value())
            {
                throw std::runtime_error("Invalid UUID");
            }

            uuid_ = js_uuid.value();

            for (auto&& comp_js : json["Components"])
            {
                if (comp_js["Type"].asString() == typeid(TransformComponent).name())
                {
                    CreateComponent<TransformComponent>(comp_js["Properties"]);
                }
                else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                {
                    CreateComponent<ConsoleComponent>(comp_js["Properties"]);
                }
            }

            for (auto&& kid_js : json["Children"])
            {
                auto new_kid = std::make_shared<GameObject>(kid_js, nullptr);
                new_kid->parent_ = std::static_pointer_cast<GameObject>(shared_from_this());
                children_.insert({new_kid->GetUuid(), new_kid});
            }

            for (auto&& kid_js : json["Children"])
            {
                children_.at(Uuid::FromString(kid_js["Uuid"].asString()).value())->Restore(kid_js);
            }
        }

        Json::Value ToJson() const override
        {
            Json::Value result;
            result["Name"] = name_;
            result["Uuid"] = uuid_.ToString();

            for (auto&& [_,component] : components_)
            {
                result["Components"].append(component->ToJson());
            }

            for (auto&& [_,child] : children_)
            {
                result["Children"].append(child->ToJson());
            }

            return result;
        }

        void Restore(Json::Value json)
        {
            for (auto&& comp_js : json["Components"])
            {
                components_.at(Uuid::FromString(comp_js["Uuid"].asString()).value())->Restore(comp_js["Properties"]);
            }
        }

        void Tick(float dt) override
        {
            for (auto&& [_,component] : components_)
            {
                component->Tick(dt);
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
            Todo(); //todo
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

        void RemoveChild(std::shared_ptr<GameObject> child)
        {
            children_.erase(child->GetUuid());
        }

        Uuid GetUuid() const
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
        std::unordered_map<Uuid, std::shared_ptr<IComponent>> components_;
        std::weak_ptr<GameObject> parent_;
        std::unordered_map<Uuid, std::shared_ptr<GameObject>> children_;
        std::weak_ptr<ILevelRootGameObjects> level_root_gos_;
        bool is_in_root = true;
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
