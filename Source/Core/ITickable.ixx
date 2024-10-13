module;

export module ITickable;

namespace GiiGa
{
    export enum TickType
    {
        None,
        Default
    };

    export class ITickable
    {
    public:
        virtual void Tick(float dt) =0;
        TickType tick_type = Default;
    };
}