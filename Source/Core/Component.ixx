module;

export module Component;

export import ITickable;

namespace GiiGa
{
    export class Component : public ITickable
    {
    public:
        virtual ~Component() = default;

    };
}
