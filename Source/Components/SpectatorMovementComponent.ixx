module;
#include <memory>

export module SpectatorMovementComponent;
import Component;
import GameObject;
import TransformComponent;
import Input;

namespace GiiGa
{
    export class SpectatorMovementComponent : public Component
    {
    public:
        void Init() override
        {
            transformOwner_ = owner_.lock()->GetTransformComponent();
        }

        void Tick(float dt) override
        {
            //Input::IsKeyDown();
        }

    protected:
        std::weak_ptr<TransformComponent> transformOwner_;
    };
}
