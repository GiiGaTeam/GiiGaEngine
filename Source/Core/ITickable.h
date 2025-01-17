#pragma once

namespace GiiGa
{
    enum TickType
    {
        None,
        Default
    };

    class ITickable
    {
    public:
        virtual ~ITickable() = default;
        virtual void Tick(float dt) =0;
        TickType tick_type = Default;
    };
}