module;

#include "directxtk12/SimpleMath.h"
#include <memory>
#include "Core/Utils.h"

export module TransformComponent;
import Component;

namespace GiiGa
{
    using namespace DirectX::SimpleMath;

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

        void Tick(float dt) override;

        void Init() override
        {

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

        std::weak_ptr<TransformComponent> GetParent() const { return parent_; }

    private:
        bool is_dirty_ = false;
        Transform transform_;
        Matrix world_matrix;
        Matrix local_matrix_;
        std::weak_ptr<TransformComponent> parent_;


        void UpdateTransformMatrix()
        {
            local_matrix_ = transform_.GetMatrix();
        }
    };
}