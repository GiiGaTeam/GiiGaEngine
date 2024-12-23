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
import Engine;
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
            //TryDoLocationMove(dt);
            //TryDoRotationMove(dt);
        }

        void TryDoLocationMove(float dt)
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();
            if (Input::IsKeyHeld(KeyCode::KeyW))
            {
                velocity += transf->GetTransform().GetForward() * speed_ * dt;
                el::Loggers::getLogger(LogMovement)->info("Movement location velocity: %v", velocity.x);
            }
            if (!Input::IsKeyHeld(KeyCode::KeyControl))
                speed_ += Input::GetMouseWhell().y * 0.5;
            speed_ = std::clamp(speed_, 0.01f, 100.0f);

            if (Input::IsKeyHeld(KeyCode::KeyA)) { velocity -= transf->GetTransform().GetRight() * speed_ * dt; }
            if (Input::IsKeyHeld(KeyCode::KeyS)) { velocity -= transf->GetTransform().GetForward() * speed_ * dt; }
            if (Input::IsKeyHeld(KeyCode::KeyD)) { velocity += transf->GetTransform().GetRight() * speed_ * dt; }
            transf->AddLocation(velocity);
        }

        void TryDoRotationMove(float dt)
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();

            if (Input::IsKeyHeld(KeyCode::KeyControl))
                cameraSensitivity_ += Input::GetMouseWhell().y;
            cameraSensitivity_ = std::clamp(cameraSensitivity_, 0.01f, 100.0f);

            const auto mouseDelta = Input::GetMouseDelta();
            velocity.x += mouseDelta.dx * cameraSensitivity_ * dt;
            velocity.y += mouseDelta.dy * cameraSensitivity_ * dt;
            transf->AddRotation(velocity);
        }

        float speed_ = 1.0f;
        float cameraSensitivity_ = 10.0f;

    protected:
        std::weak_ptr<TransformComponent> transformOwner_;
    };
}
