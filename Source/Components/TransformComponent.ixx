module;

#include "directxtk12/SimpleMath.h"
#include <memory>
#include "Core/Utils.h"

export module TransformComponent;
import Component;
import EventSystem;

namespace GiiGa
{
    using namespace DirectX::SimpleMath;

    struct UpdateTransformEvent
    {
    };

#pragma region TransfromStructure
    struct Transform
    {
    public:
        Transform(Vector3 loc = Vector3::Zero, Vector3 rot = Vector3::Zero, Vector3 scale = Vector3::One)
            : location_(loc), scale_(scale)
        {
            rotate_ = Quaternion::CreateFromYawPitchRoll(RadFromDeg(rot));
        }

        Transform(const Transform& transform) = default;

        Transform(Transform&& transform) noexcept
            : location_(transform.location_), rotate_(transform.rotate_), scale_(transform.scale_)
        {
            transform = Identity;
        }

        Transform& operator=(const Transform& other)
        {
            if (*this == other) return *this;
            Transform temp(other);
            std::swap(this->location_, temp.location_);
            std::swap(this->rotate_, temp.rotate_);
            std::swap(this->scale_, temp.scale_);
            return *this;
        }

        Transform& operator=(Transform&& other) noexcept
        {
            if (*this == other) return *this;

            std::swap(this->location_, other.location_);
            std::swap(this->rotate_, other.rotate_);
            std::swap(this->scale_, other.scale_);
            other = Identity;
            return *this;
        }

        bool operator==(const Transform& rhs) const
        {
            return location_ == rhs.location_ && rotate_ == rhs.rotate_ && scale_ == rhs.scale_;
        }

        Vector3 GetUp() const
        {
            return Matrix::CreateFromQuaternion(rotate_).Up();
        }

        Vector3 GetForward() const
        {
            return Matrix::CreateFromQuaternion(rotate_).Forward();
        }

        Vector3 GetRight() const
        {
            return Matrix::CreateFromQuaternion(rotate_).Right();
        }

        Matrix GetMatrix() const
        {
            Matrix resMat;
            resMat *= resMat.CreateScale(scale_);
            resMat *= Matrix::CreateFromQuaternion(rotate_);
            resMat.Translation(location_);
            return resMat;
        }

        static Transform TransformFromMatrix(const Matrix& mTrans)
        {
            Transform resTrans;
            Matrix cop = mTrans;
            cop.Decompose(resTrans.scale_, resTrans.rotate_, resTrans.location_);
            return resTrans;
        }

        Vector3 GetRotation() const { return DegFromRad(rotate_.ToEuler()); }
        void SetRotation(const Vector3& rot) { rotate_ = Quaternion::CreateFromYawPitchRoll(RadFromDeg(rot)); }

        static const Transform Identity;

        Quaternion rotate_ = Quaternion::Identity;
        Vector3 location_ = Vector3::Zero;
        Vector3 scale_ = Vector3::One;
    };

    const Transform Transform::Identity = Transform();
#pragma endregion

#pragma region TransformComopnent
    export class TransformComponent : public Component, public std::enable_shared_from_this<GameObject>
    {

    public:
        TransformComponent(const Vector3 location = Vector3::Zero
            , const Vector3 rotation = Vector3::Zero
            , const Vector3 scale = Vector3::One
            , const std::shared_ptr<TransformComponent>& parent = nullptr)
        {
            transform_ = Transform{location, rotation, scale};
            if (parent) AttachTo(parent);
        }

        TransformComponent(const Transform& transform, const std::shared_ptr<TransformComponent>& parent = nullptr)
        {
            transform_ = transform;
            if (parent) AttachTo(parent);
        }

        TransformComponent(const std::weak_ptr<TransformComponent>& other)
        {
            transform_ = other.lock()->transform_;
            parent_ = other.lock()->parent_;
        }

        TransformComponent* operator=(const std::weak_ptr<TransformComponent>& other)
        {
            if (this == other.lock().get()) return this;
            std::weak_ptr temp(other);
            std::swap(this->transform_, temp.lock()->transform_);
            std::swap(this->parent_, temp.lock()->parent_);
            return this;
        }

        bool operator==(const TransformComponent* rhs) const
        {
            return transform_ == rhs->transform_;
        }

        void Tick(float dt) override
        {
            Component::Tick(dt);
            if (is_dirty_) UpdateTransformMatrices();
        }

        void Init() override
        {
            Component::Init();
            AttachTo(parent_);
        }

        Transform GetTransform() const { return transform_; }

        void SetTransform(const Transform& transform)
        {
            transform_ = transform;
            UpdateTransformMatrices();
        }

        Vector3 GetLocation() const { return transform_.location_; }

        void SetLocation(const Vector3& location)
        {
            transform_ = std::move(Transform(location));
            UpdateTransformMatrices();
        }

        Vector3 GetRotation() const { return transform_.GetRotation(); }

        void SetRotation(const Vector3& rotation)
        {
            transform_.SetRotation(rotation);
            UpdateTransformMatrices();
        }

        Vector3 GetScale() const { return transform_.scale_; }

        void SetScale(const Vector3& scale)
        {
            transform_.scale_ = scale;
            UpdateTransformMatrices();
        }

        Transform GetWorldTransform() const { return Transform::TransformFromMatrix(world_matrix_); }

        void SetWorldTransform(const Transform& transform)
        {
            const auto pref_world_matrix = world_matrix_;
            world_matrix_ = transform.GetMatrix();
            Matrix world_to_local = pref_world_matrix.Invert() * local_matrix_;
            local_matrix_ = world_matrix_ * world_to_local;
            transform_ = Transform::TransformFromMatrix(local_matrix_);
            OnUpdateTransform.Invoke(UpdateTransformEvent{});
        }

        Vector3 GetWorldLocation() const
        {
            return Transform::TransformFromMatrix(world_matrix_).location_;
        }

        void SetWorldLocation(const Vector3& location)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.location_ = location;
            SetWorldTransform(world_trans);
        }

        Vector3 GetWorldRotation() const
        {
            return Transform::TransformFromMatrix(world_matrix_).GetRotation();
        }

        void SetWorldRotation(const Vector3& rotation)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.SetRotation(rotation);
            SetWorldTransform(world_trans);
        }

        Vector3 GetWorldScale() const
        {
            return Transform::TransformFromMatrix(world_matrix_).scale_;
        }

        void SetWorldScale(const Vector3& scale)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.scale_ = scale;
            SetWorldTransform(world_trans);
        }

        std::weak_ptr<TransformComponent> GetParent() const { return parent_; }

        void AttachTo(const std::weak_ptr<TransformComponent>& parent)
        {
            if (parent.expired()) return;
            if (!parent_.expired()) Detach();
            parent_ = parent;
            cashed_event_ = parent_.lock()->OnUpdateTransform.Register([this](const UpdateTransformEvent& e) { ParentUpdateTransform(e); });
            UpdateTransformMatrices();
        }

        void Detach()
        {
            if (parent_.expired()) return;
            parent_.lock()->OnUpdateTransform.Unregister(cashed_event_);
            parent_.reset();
            UpdateTransformMatrices();
        }

        EventDispatcher<UpdateTransformEvent> OnUpdateTransform;

        bool attach_translation = true;
        bool attach_rotate = true;
        bool attach_scale = true;

    private:
        bool is_dirty_ = true;
        Transform transform_;
        Matrix world_matrix_;
        Matrix local_matrix_;
        std::weak_ptr<TransformComponent> parent_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();

        void UpdateTransformMatrices()
        {
            const auto pref_local_matrix = local_matrix_;
            local_matrix_ = transform_.GetMatrix();
            if (!is_dirty_)
            {
                Matrix local_to_world = pref_local_matrix.Invert() * world_matrix_;
                world_matrix_ = local_matrix_ * local_to_world;
            }
            else
            {
                CalcWorldTransformMatrix();
            }
            OnUpdateTransform.Invoke(UpdateTransformEvent{});
        }

        void CalcWorldTransformMatrix()
        {
            const TransformComponent* rootComp = this;
            world_matrix_ = rootComp->local_matrix_;

            const TransformComponent* parentComp = rootComp->parent_.lock().get();
            while (parentComp)
            {
                if (rootComp->attach_translation && rootComp->attach_rotate && rootComp->attach_scale)
                {
                    world_matrix_ *= parentComp->local_matrix_;
                }
                else if (rootComp->attach_translation && rootComp->attach_rotate)
                {
                    world_matrix_.Translation(parentComp->local_matrix_.Translation());
                    world_matrix_ *= world_matrix_.CreateFromYawPitchRoll(parentComp->local_matrix_.ToEuler());
                }
                else if (rootComp->attach_translation && rootComp->attach_scale)
                {
                    world_matrix_.Translation(parentComp->local_matrix_.Translation());
                    world_matrix_ *= world_matrix_.CreateScale(parentComp->transform_.scale_);
                }
                else if (rootComp->attach_rotate && rootComp->attach_scale)
                {
                    world_matrix_ *= world_matrix_.CreateFromYawPitchRoll(parentComp->local_matrix_.ToEuler());
                    world_matrix_ *= world_matrix_.CreateScale(parentComp->transform_.scale_);
                }
                else if (rootComp->attach_translation)
                {
                    world_matrix_.Translation(parentComp->local_matrix_.Translation());
                }
                else if (rootComp->attach_rotate)
                {
                    world_matrix_ *= world_matrix_.CreateFromYawPitchRoll(parentComp->local_matrix_.ToEuler());
                }
                else if (rootComp->attach_scale)
                {
                    world_matrix_ *= world_matrix_.CreateScale(parentComp->transform_.scale_);
                }

                rootComp = parentComp;
                parentComp = rootComp->parent_.lock().get();
            }
        }

        void ParentUpdateTransform(const UpdateTransformEvent& e)
        {
            is_dirty_ = true;
            UpdateTransformMatrices();
        }
    };
#pragma endregion

}