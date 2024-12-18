module;

#include <vector>
#include <memory>
#include <typeindex>
#include <directxtk12/SimpleMath.h>
#include <json/json.h>

export module GameObject;

import Component;
import TransformComponent;
import Uuid;
using namespace DirectX::SimpleMath;

namespace GiiGa
{
    export class GameObject final : public ITickable, public std::enable_shared_from_this<GameObject>
    {
    public:
        explicit GameObject(const Vector3 location = Vector3::Zero
                           , const Vector3 rotation = Vector3::Zero
                           , const Vector3 scale = Vector3::One)
        {
            tempInit_ = Transform(location, rotation, scale);
        }

        void Tick(float dt) override
        {
            for (auto&& component : components_)
            {
                component->Tick(dt);
            }
        }

        void Init()
        {            
            transform_ = CreateComponent<TransformComponent>(tempInit_);
            if (transform_.expired()) transform_ = CreateComponent<TransformComponent>();

            for (auto&& component : components_)
            {
                component->Init();
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

        std::weak_ptr<TransformComponent> GetTransformComponent()
        {
            return transform_;
        }

        void AddComponent(std::shared_ptr<Component> component)
        {
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

        std::vector<std::shared_ptr<Component>> GetComponents()
        {
            return components_;
        }

        Uuid GetUuid() const
        {
            // TO DO
            Uuid::Null();
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
                componentJson.append(component->ToJSon().toStyledString());
                componentsResult.append(componentJson);
            }
            result["Components"] = componentsResult.toStyledString();

            return result;
        }

    private:
        std::vector<std::shared_ptr<Component>> components_;
        std::weak_ptr<TransformComponent> transform_;
        Transform tempInit_;
    };
} // namespace GiiGa
