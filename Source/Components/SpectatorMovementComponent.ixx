export module SpectatorMovementComponent;

import <memory>;
import <directxtk12/SimpleMath.h>;
import <json/value.h>;
import <imgui.h>;
import <optional>;

import Component;
import GameObject;
import TransformComponent;
import Input;
import Window;
import Misc;
import Logger;
import Engine;
import PrefabInstance;

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

        void BeginPlay() override
        {
            
        }

        void Restore(const ::Json::Value&) override { Todo(); }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>
                                          & instance_uuid) override
        {
            Todo();
            return {};
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            Todo();
        }


        ::Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Todo();
            return Json::Value();
        };

        void Tick(float dt) override
        {
            if (!active_) return;
            TryDoLocationMove(dt);
            TryDoRotationMove(dt);
        }

        void TryDoLocationMove(float dt)
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();
            if (!Input::IsKeyHeld(KeyCode::KeyControl))
                speed_ += Input::GetMouseWhell().y * 0.5;
            speed_ = std::clamp(speed_, 0.01f, 100.0f);

            if (Input::IsKeyHeld(KeyCode::KeyW)) velocity += transf->GetTransform().GetForward() * speed_ * dt;
            if (Input::IsKeyHeld(KeyCode::KeyA)) { velocity -= transf->GetTransform().GetRight() * speed_ * dt; }
            if (Input::IsKeyHeld(KeyCode::KeyS)) { velocity -= transf->GetTransform().GetForward() * speed_ * dt; }
            if (Input::IsKeyHeld(KeyCode::KeyD)) { velocity += transf->GetTransform().GetRight() * speed_ * dt; }
            if (Input::IsKeyHeld(KeyCode::KeyE)) { velocity += transf->GetTransform().GetUp() * speed_ * dt; }
            if (Input::IsKeyHeld(KeyCode::KeyQ)) { velocity -= transf->GetTransform().GetUp() * speed_ * dt; }
            transf->AddLocation(velocity);
        }

        void TryDoRotationMove(float dt)
        {
            if (transformOwner_.expired()) return;
            Vector3 velocity = Vector3::Zero;
            const auto transf = transformOwner_.lock();

            if (Input::IsKeyHeld(KeyCode::KeyControl))
                cameraSensitivity_ += Input::GetMouseWhell().y;
            cameraSensitivity_ = std::clamp(cameraSensitivity_, 0.01f, 1000.0f);

            const auto mouseDelta = Input::GetMouseDelta();
            velocity.x -= mouseDelta.dy * cameraSensitivity_;
            velocity.y -= mouseDelta.dx * cameraSensitivity_;
            velocity = Vector3(velocity.x, velocity.y, 0) * dt;
            transf->AddRotation(velocity);
        }

        float speed_ = 1.0f;
        float cameraSensitivity_ = 50.0f;

        bool active_ = false;

    protected:
        std::weak_ptr<TransformComponent> transformOwner_;
    };
}
