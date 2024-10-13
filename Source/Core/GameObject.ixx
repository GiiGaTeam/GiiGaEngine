module;

#include<vector>
#include<memory>

export module GameObject;

import Component;

namespace GiiGa
{
    export class GameObject final : public ITickable
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
                //newComp->Owner = Owner;
                components_.push_back(newComp);
                return newComp;
            }
            return nullptr;
        }

    private:
        std::vector<std::shared_ptr<Component>> components_;
    };
}