module;
#include <memory>
#include <directxtk12/SimpleMath.h>

export module SpectatorMovementComponent;
import <json/value.h>;
import Component;
import GameObject;
import TransformComponent;
import Input;
import Window;
import Misc;

using namespace DirectX::SimpleMath;

namespace GiiGa
{
    export class SpectatorMovementComponent : public Component
    {
    public:
        void Init() override
        {
            const auto GO = std::dynamic_pointer_cast<GameObject>(owner_.lock());
            transformOwner_ = GO->GetTransformComponent();
        }

        void Restore(const ::Json::Value&) override { Todo(); }

        std::shared_ptr<IComponent> Clone() override
        {
            Todo();
            return nullptr;
        }

        ::Json::Value DerivedToJson() override
        {
            Todo();
            return Json::Value();
        };

        void Tick(float dt) override
        {
            TryDoLocationMove();
            TryDoRotationMove();
        }

        void TryDoLocationMove() const
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();
            if (Input::IsKeyDown(KeyCode::KeyW)) { velocity += transf->GetTransform().GetForward() * speed; }
            if (Input::IsKeyDown(KeyCode::KeyS)) { velocity -= transf->GetTransform().GetForward() * speed; }
            if (Input::IsKeyDown(KeyCode::KeyD)) { velocity += transf->GetTransform().GetRight() * speed; }
            if (Input::IsKeyDown(KeyCode::KeyA)) { velocity -= transf->GetTransform().GetRight() * speed; }
            transf->AddLocation(velocity);
        }

        void TryDoRotationMove() const
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();

            Input::GetMouseDelta();
            transf->AddRotation(velocity);
        }

        float speed = 1.0f;
        float camera_sensitivity = 0.1f;

    protected:
        std::weak_ptr<TransformComponent> transformOwner_;
    };
}
