module;

export module Component;

export import ITickable;
#include <memory>

namespace GiiGa
{
    export class GameObject;

    export class Component : public ITickable
    {
    public:
        virtual ~Component() = default;

        virtual void Init() = 0;

    protected:
        friend class GameObject;
        std::weak_ptr<GameObject> owner_;
    };
}  // namespace GiiGa
