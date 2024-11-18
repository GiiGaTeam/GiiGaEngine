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

    struct UpdateParentEvent
    {
    };

    struct Transform
    {
    public:
        Transform(Vector3 loc = Vector3::Zero, Vector3 rot = Vector3::Zero, Vector3 scale = Vector3::One)
            : location(loc), scale(scale)
        {
            rotate = Quaternion::CreateFromYawPitchRoll(RadFromDeg(rot));
        }

        Transform(const Transform& transform) = default;

        Transform(Transform&& transform) noexcept
            : location(transform.location), rotate(transform.rotate), scale(transform.scale)
        {
            transform = Identity;
        }

        bool operator==(const Transform& rhs) const
        {
            return location == rhs.location && rotate == rhs.rotate && scale == rhs.scale;
        }

        Transform& operator=(const Transform& other)
        {
            if (*this == other) return *this;
            Transform temp(other);
            std::swap(this->location, temp.location);
            std::swap(this->rotate, temp.rotate);
            std::swap(this->scale, temp.scale);
            return *this;
        }

        Transform& operator=(Transform&& other) noexcept
        {
            if (*this == other) return *this;

            std::swap(this->location, other.location);
            std::swap(this->rotate, other.rotate);
            std::swap(this->scale, other.scale);
            other = Identity;
            return *this;
        }

        Vector3 GetUp() const
        {
            return Matrix::CreateFromQuaternion(rotate).Up();
        }

        Vector3 GetForward() const
        {
            return Matrix::CreateFromQuaternion(rotate).Forward();
        }

        Vector3 GetRight() const
        {
            return Matrix::CreateFromQuaternion(rotate).Right();
        }

        Matrix GetMatrix() const
        {
            Matrix resMat;
            resMat *= resMat.CreateScale(scale);
            resMat *= Matrix::CreateFromQuaternion(rotate);
            resMat.Translation(location);
            return resMat;
        }

        static Transform TransformFromMatrix(const Matrix& mTrans)
        {
            Transform resTrans;
            Matrix cop = mTrans;
            cop.Decompose(resTrans.scale, resTrans.rotate, resTrans.location);
            return resTrans;
        }

        Vector3 Location() const { return location; }
        Vector3 Rotation() const { return DegFromRad(rotate.ToEuler()); }
        Vector3 Scale() const { return scale; }

        static const Transform Identity;

    private:
        Quaternion rotate = Quaternion::Identity;
        Vector3 location = Vector3::Zero;
        Vector3 scale = Vector3::One;
    };

    const Transform Transform::Identity = Transform();

    export class TransformComponent : public Component
    {
    public:
        TransformComponent(const Vector3 location = Vector3::Zero
            , const Vector3 rotation = Vector3::Zero
            , const Vector3 scale = Vector3::One)
        {
            transform_ = Transform{location, rotation, scale};
        }

        TransformComponent(Transform&& transform)
        {
            transform_ = transform;
        }

        void Tick(float dt) override
        {
            if (is_dirty_) UpdateTransformMatrix();
        }

        void Init() override
        {
            OnUpdateParentEvent.Register([this](const UpdateParentEvent& e) { OnUpdateParent(e); });
            //OnUpdateParentEvent.Register(std::function<void(const UpdateParentEvent&)>(&TransformComponent::OnUpdateParent));
        }

        Transform GetTransform() const { return transform_; }

        void SetTransform(const Transform& transform)
        {
            transform_ = transform;
            UpdateTransformMatrix();
        }

        Transform GetWorldTransform() const { return Transform::TransformFromMatrix(world_matrix); }

        void SetWorldTransform()
        {

        }

        Vector3 GetLocation() const { return transform_.Location(); }

        void SetLocation(const Vector3& location)
        {
            transform_ = Transform(location);
            UpdateTransformMatrix();
        }

        Vector3 GetRotation() const { return transform_.Rotation(); }

        void SetRotation(const Vector3& rotation)
        {
            transform_ = Transform(transform_.Location(), rotation);
            UpdateTransformMatrix();
        }

        Vector3 GetScale() const { return transform_.Scale(); }

        void SetScale(const Vector3& scale)
        {
            transform_ = Transform(transform_.Location(), transform_.Rotation(), scale);
            UpdateTransformMatrix();
        }

        Vector3 GetWorldLocation() const { return transform_.Location(); }

        void SetWorldLocation(const Vector3& location)
        {
        }

        Vector3 GetWorldRotation() const { return transform_.Rotation(); }

        void SetWorldRotation(const Vector3& rotation)
        {
        }

        Vector3 GetWorldScale() const { return transform_.Scale(); }

        void SetWorldScale(const Vector3& scale)
        {
        }


        std::weak_ptr<TransformComponent> GetParent() const { return parent_; }

        void AttachTo(std::weak_ptr<TransformComponent> parent)
        {
            if (parent.expired()) return;
            parent_ = parent;
        }

        void Detach()
        {
            parent_.reset();

        }

        EventDispatcher<UpdateParentEvent> OnUpdateParentEvent;

    private:
        bool is_dirty_ = false;
        Transform transform_;
        Matrix world_matrix;
        Matrix local_matrix_;
        std::weak_ptr<TransformComponent> parent_;


        void UpdateTransformMatrix()
        {
            local_matrix_ = transform_.GetMatrix();
            world_matrix;
            is_dirty_ = false;
        }

        void OnUpdateParent(const UpdateParentEvent& e)
        {

        };
    };
}