module;

#include <vector>
#include <memory>
#include <typeindex>
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
import IComponentsInLevel;
import ILevelRootGameObjects;

namespace GiiGa
{
    // life time of one GameObject is bound to parent
    // cause it has shared_ptr to GameObject
    // note: if GameObject does not has parent he should be in Levels root GameObjects list
    export class GameObject final : public IGameObject
    {
    public:
        GameObject(std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr, std::shared_ptr<IComponentsInLevel> level_comps = nullptr):
            level_root_gos_(level_rgo),
            level_components_(level_comps)
        {
            if (level_rgo)
                level_rgo->AddRootGameObject(shared_from_this());
        }

        void Destroy()
        {
            if (auto l_level = level_root_gos_.lock())
            {
                l_level->RemoveRootGameObject(shared_from_this());
            }

            for (auto kid : children_)
            {
                kid->Destroy();
            }

            for (auto comp : components_)
            {
                comp->Destroy();
            }
        }

        ~GameObject() override = default;

        GameObject(const GameObject& other) = delete;
        GameObject(GameObject&& other) noexcept = default;
        GameObject& operator=(const GameObject& other) = delete;
        GameObject& operator=(GameObject&& other) noexcept = default;

        GameObject(const Uuid& uuid, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr, std::shared_ptr<IComponentsInLevel> level_comps = nullptr):
            GameObject(level_rgo, level_comps)
        {
            uuid_ = uuid;
        }

        /* GameObject Json
         * Name:
         * Uuid:
         * Components:[...]
         * Children:[...]
        */
        GameObject(Json::Value json, std::shared_ptr<ILevelRootGameObjects> level_rgo = nullptr, std::shared_ptr<IComponentsInLevel> level_comps = nullptr):
            GameObject(level_rgo, level_comps)
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
                    CreateComponent<TransformComponent>(comp_js["Properties"], level_comps);
                }
                else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                {
                    CreateComponent<ConsoleComponent>(comp_js["Properties"], level_comps);
                }
            }

            for (auto&& kid_js : json["Children"])
            {
                auto new_kid = std::make_shared<GameObject>(kid_js, nullptr, level_comps);
                new_kid->parent_ = std::static_pointer_cast<GameObject>(shared_from_this());
                children_.insert(new_kid);
            }
        }

        Json::Value ToJson() const override
        {
            Json::Value result;
            result["Name"] = name_;
            result["Uuid"] = uuid_.ToString();

            for (auto component : components_)
            {
                result["Components"].append(component->ToJson());
            }

            for (auto child : children_)
            {
                result["Children"].append(child->ToJson());
            }

            return result;
        }

        void Tick(float dt) override
        {
            for (auto&& component : components_)
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
                components_.insert(newComp);
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

        void AddComponent(std::shared_ptr<IComponent> component) override
        {
            component->SetOwner(shared_from_this());
            components_.insert(component);
        }

        void SetParent(std::shared_ptr<GameObject> parent, bool safeWorldTransfrom)
        {
            Todo(); //todo
        }

        virtual std::shared_ptr<GameObject> Clone()
        {
            auto newGameObject = std::make_shared<GameObject>();
            for (auto component : components_)
            {
                newGameObject->AddComponent(component->Clone());
            }
            return newGameObject;
        }

        std::unordered_set<std::shared_ptr<IComponent>>& GetComponents()
        {
            return components_;
        }

        const std::unordered_set<std::shared_ptr<GameObject>>& GetChildren() const
        {
            return children_;
        }

        void RemoveChild(std::shared_ptr<GameObject> child)
        {
            children_.erase(child);
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
        std::unordered_set<std::shared_ptr<IComponent>> components_;
        std::weak_ptr<GameObject> parent_;
        std::unordered_set<std::shared_ptr<GameObject>> children_;
        std::weak_ptr<ILevelRootGameObjects> level_root_gos_;
        std::weak_ptr<IComponentsInLevel> level_components_;
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
