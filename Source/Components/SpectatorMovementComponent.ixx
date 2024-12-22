export module SpectatorMovementComponent;

import <memory>;
import <directxtk12/SimpleMath.h>;
import <json/value.h>;
import Component;
import GameObject;
import TransformComponent;
import Input;
import Window;
import Misc;
import Logger;

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
            TryDoLocationMove(dt);
            TryDoRotationMove(dt);
        }

        void TryDoLocationMove(float dt) const
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();
            if (Input::IsKeyDown(KeyCode::KeyW)) { velocity += transf->GetTransform().GetForward() * speed_ * dt; }
            if (Input::IsKeyDown(KeyCode::KeyA)) { velocity -= transf->GetTransform().GetRight() * speed_ * dt; }
            if (Input::IsKeyDown(KeyCode::KeyS)) { velocity -= transf->GetTransform().GetForward() * speed_ * dt; }
            if (Input::IsKeyDown(KeyCode::KeyD)) { velocity += transf->GetTransform().GetRight() * speed_ * dt; }
            el::Loggers::getLogger(LogMovement)->info("Movement location velocity: %v", velocity.x);
            transf->AddLocation(velocity);
        }

        void TryDoRotationMove(float dt) const
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();

            const auto mouseDelta = Input::GetMouseDelta();
            velocity.x += mouseDelta.dx * cameraSensitivity_ * dt;
            velocity.y += mouseDelta.dy * cameraSensitivity_ * dt;
            transf->AddRotation(velocity);
        }

        float speed_ = 1.0f;
        float cameraSensitivity_ = 0.1f;

    protected:
        std::weak_ptr<TransformComponent> transformOwner_;
    };
}
