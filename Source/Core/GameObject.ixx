module;

#include <vector>
#include <memory>

export module GameObject;

import Component;

namespace GiiGa
{
    export class GameObject final : public ITickable, public std::enable_shared_from_this<GameObject>
    {
    public:
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
                newComp->owner_ = shared_from_this();
                components_.push_back(newComp);
                return newComp;
            }
            return nullptr;
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

    private:
        std::vector<std::shared_ptr<Component>> components_;
    };
} // namespace GiiGa
