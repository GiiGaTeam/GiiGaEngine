module;

#include <vector>
#include <memory>
#include <typeindex>
#include <json/json.h>

export module GameObject;

export import IGameObject;
import IComponent;
import ITickable;
import Uuid;

namespace GiiGa
{
    export class GameObject final : public IGameObject
    {
    public:
        GameObject() = default;
        void Tick(float dt) override
        {
            for (auto&& component : components_)
            {
                component->Tick(dt);
            }
        }

        GameObject(Json::Value json)
        {
            name_ = json["name"].asString();
            
            for (auto&& comp_js : json["Components"])
            {
                //if (comp_js["Type"].asString() == typeid(TransformComponent).name())
                //{
                //    AddComponent(std::make_shared<TransformComponent>(comp_js["Properties"]));
                //}
                //else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                //{
                //    AddComponent(std::make_shared<ConsoleComponent>(comp_js["Properties"]));
                //}
            }

            for (auto&& kid_js: json["Children"])
            {
                children_.push_back(std::make_shared<GameObject>(kid_js));
            }
        }

        template <typename T, typename... Args>
        std::shared_ptr<T> CreateComponent(const Args&... args)
        {
            if (std::shared_ptr<T> newComp = std::make_shared<T>(args...))
            {
                newComp->SetOwner(shared_from_this());
                components_.push_back(newComp);
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

        void AddComponent(std::shared_ptr<IComponent> component)override
        {
            component->SetOwner(shared_from_this());
            components_.push_back(component);
        }

        void SetParent(std::shared_ptr<GameObject> parent, bool safeWorldTransfrom)
        {
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

        const std::vector<std::shared_ptr<IComponent>>& GetComponents() const
        {
            return components_;
        }

        const std::vector<std::shared_ptr<GameObject>>& GetChildren() const
        {
            return children_;
        }

        Uuid GetUuid() const
        {
            return uuid_;
        }

        Json::Value ToJson()
        {
            Json::Value result;
            result["Name"] = "";
            Json::Value componentsResult;
            for (auto component : components_)
            {
                Json::Value componentJson;
                componentJson["component"] = typeid(component).name();
                componentJson.append(component->ToJson().toStyledString());
                componentsResult.append(componentJson);
            }
            result["Components"] = componentsResult.toStyledString();

            return result;
        }


    private:
        Uuid uuid_ = Uuid::Null();
        std::string name_;
        std::vector<std::shared_ptr<IComponent>> components_;
        std::vector<std::shared_ptr<GameObject>> children_;
        //std::weak_ptr<Level> level_;
    };
} // namespace GiiGa
