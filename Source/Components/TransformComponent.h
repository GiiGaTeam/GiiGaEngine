﻿#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<memory>
#include<optional>

#include<DXMathUtils.h>
#include<Component.h>
#include<EventSystem.h>
#include<IWorldQuery.h>
#include<Logger.h>
#include<PrefabInstanceModifications.h>

namespace GiiGa
{
    using namespace DirectX::SimpleMath;


#pragma region TransfromStructure
    struct Transform
    {
    public:
        Transform(Vector3 loc = Vector3::Zero, Vector3 rot = Vector3::Zero, Vector3 scale = Vector3::One)
            : location_(loc), scale_(scale)
        {
            SetRotation(rot);
        }

        Transform(Vector3 loc, Quaternion rot, Vector3 scale)
            : location_(loc), rotate_(rot), scale_(scale)
        {
        }

        Transform(Json::Value json)
            : location_(Vector3FromJson(json["Location"])), scale_(Vector3FromJson(json["Scale"]))
        {
            SetRotation(Vector3FromJson(json["Rotation"]));
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

        bool operator!=(const Transform& rhs) const
        {
            return !(*this == rhs);
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

        Json::Value ToJson() const
        {
            Json::Value res;

            res["Location"] = Vector3ToJson(location_);
            res["Rotation"] = Vector3ToJson(GetRotation());
            res["Scale"] = Vector3ToJson(scale_);

            return res;
        }

        static Transform TransformFromMatrix(const Matrix& mTrans)
        {
            Transform resTrans;
            Matrix cop = mTrans;
            cop.Decompose(resTrans.scale_, resTrans.rotate_, resTrans.location_);
            return resTrans;
        }

        Vector3 GetRotation() const
        {
            auto temp_rot = DegFromRad(rotate_.ToEuler());
            //temp_rot = Vector3(temp_rot.y, temp_rot.x, temp_rot.z);
            return temp_rot;
        }

        void SetRotation(const Vector3& rot)
        {
            const auto rad_rot = RadFromDeg(rot);
            rotate_ = Quaternion::CreateFromYawPitchRoll(RadFromDeg(rot));
        }

        static Quaternion QuatFromRot(const Vector3& rot)
        {
            const auto rad_rot = RadFromDeg(rot);
            return Quaternion::CreateFromYawPitchRoll(RadFromDeg(rot));
        }

        static const Transform Identity;

        Quaternion rotate_ = Quaternion::Identity;
        Vector3 location_ = Vector3::Zero;
        Vector3 scale_ = Vector3::One;
    };

    const Transform Transform::Identity = Transform();
#pragma endregion

#pragma region TransformComopnent
    // todo: add to json
    class TransformComponent : public Component
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

        TransformComponent(const Json::Value& json, bool roll_id = false):
            Component(json, roll_id)
        {
            transform_ = Transform(json["Transform"]);
        }

        TransformComponent* operator=(const std::weak_ptr<TransformComponent>& other)
        {
            if (this == other.lock().get()) return this;
            std::weak_ptr temp(other);
            std::swap(this->transform_, temp.lock()->transform_);
            std::swap(this->parent_, temp.lock()->parent_);
            return this;
        }

        std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent> prefab_comp) const override
        {
            std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> result;

            auto prefab_trans = std::static_pointer_cast<TransformComponent>(prefab_comp);

            if (this->transform_ != prefab_trans->transform_)
                result.push_back({{this->inprefab_uuid_, "Transform"}, this->transform_.ToJson()});

            if (!this->parent_.expired() && !prefab_trans->parent_.expired())
            {
                if (this->parent_.lock()->GetInPrefabUuid() != prefab_trans->parent_.lock()->GetInPrefabUuid())
                    result.push_back({{this->inprefab_uuid_, "Parent"}, this->parent_.lock()->GetUuid().ToString()});
            }
            else
            {
                Uuid parent_id = Uuid::Null();
                if (auto l_parent = this->parent_.lock())
                    parent_id = l_parent->GetUuid();
                result.push_back({{this->inprefab_uuid_, "Parent"}, parent_id.ToString()});
            }

            return result;
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            if (modifications.contains({this->inprefab_uuid_, "Transform"}))
                this->transform_ = Transform{modifications.at({this->inprefab_uuid_, "Transform"})};

            if (modifications.contains({this->inprefab_uuid_, "Parent"}))
            {
                Uuid parent_comp_uuid = Uuid::FromString(modifications.at({this->inprefab_uuid_, "Parent"}).asString()).value();

                if (parent_comp_uuid == Uuid::Null())
                {
                    Detach();
                }
                else
                {
                    AttachTo(WorldQuery::GetWithUUID<TransformComponent>(parent_comp_uuid));
                }
            }
        }

        bool operator==(const TransformComponent* rhs) const
        {
            return transform_ == rhs->transform_;
        }

        Json::Value DerivedToJson(bool is_prefab_root = false) override
        {
            Json::Value result;

            result["Type"] = typeid(TransformComponent).name();

            if (!is_prefab_root)
                result["Transform"] = transform_.ToJson();
            else
                result["Transform"] = Transform{}.ToJson();

            if (!parent_.expired() && !is_prefab_root)
                result["Parent"] = parent_.lock()->GetUuid().ToString();
            else
                result["Parent"] = Uuid::Null().ToString();

            return result;
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid,
                                          const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            auto clone = std::make_shared<TransformComponent>();
            this->CloneBase(clone, original_uuid_to_world_uuid, instance_uuid);
            clone->transform_ = transform_;
            return clone;
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid) override
        {
            auto orig_trans = std::static_pointer_cast<TransformComponent>(original);
            auto parentUuid = Uuid::Null();
            if (!orig_trans->parent_.expired())
            {
                el::Loggers::getLogger(LogWorld)->debug("TrasformComp::RestoreFromOriginal() Request %v from original_uuid_to_world_uuid", orig_trans->parent_.lock()->GetUuid().ToString());
                parentUuid = original_uuid_to_world_uuid.at(orig_trans->parent_.lock()->GetUuid());
            }
            if (parentUuid != Uuid::Null())
                AttachTo(WorldQuery::GetWithUUID<TransformComponent>(parentUuid));
        }

        void RestoreAsPrefab(const Json::Value& json, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            auto prefab_parentUuid = Uuid::FromString(json["Parent"].asString()).value();

            if (prefab_parentUuid != Uuid::Null())
            {
                // todo: sphere root has parent but should not
                Uuid world_parentUuid = prefab_uuid_to_world_uuid.at(prefab_parentUuid);
                auto world_parent = WorldQuery::GetWithUUID<TransformComponent>(world_parentUuid);
                AttachTo(world_parent);
            }
        }

        void RestoreFromLevelJson(const Json::Value& json) override
        {
            auto parentUuid = Uuid::FromString(json["Parent"].asString()).value();

            if (parentUuid != Uuid::Null())
                AttachTo(WorldQuery::GetWithUUID<TransformComponent>(parentUuid));
        }

        void Tick(float dt) override
        {
            if (is_dirty_) UpdateTransformMatrices();
        }

        void Init() override
        {
            AttachTo(parent_);
        }

        void BeginPlay() override
        {
        }

        Transform GetTransform() const { return transform_; }

        virtual void SetTransform(const Transform& transform)
        {
            transform_ = transform;
            UpdateTransformMatrices();
        }

        Vector3 GetLocation() const { return transform_.location_; }

        virtual void SetLocation(const Vector3& location)
        {
            transform_.location_ = location;
            UpdateTransformMatrices();
        }

        virtual void AddLocation(const Vector3& location)
        {
            const auto new_location = GetLocation() + location;
            SetLocation(new_location);
        }

        Vector3 GetRotation() const { return transform_.GetRotation(); }

        virtual void SetRotation(const Vector3& rotation)
        {
            transform_.SetRotation(rotation);
            UpdateTransformMatrices();
        }

        virtual void SetRotation(const Quaternion& rotation)
        {
            transform_.rotate_ = rotation;
            UpdateTransformMatrices();
        }

        virtual void AddRotation(const Vector3& rotation)
        {
            auto rot = GetRotation() + rotation;
            SetRotation(rot);
        }

        void AddQuatRotation(const Vector3& rotation)
        {
            auto q_new = Transform::QuatFromRot(rotation);
            auto rot = q_new * transform_.rotate_;
            rot.Normalize();
            SetRotation(rot);
        }

        Vector3 GetScale() const { return transform_.scale_; }

        virtual void SetScale(const Vector3& scale)
        {
            transform_.scale_ = scale;
            UpdateTransformMatrices();
        }

        virtual void AddScale(const Vector3& scale)
        {
            const auto new_scale = GetScale() + scale;
            SetScale(new_scale);
        }

        Transform GetWorldTransform() const { return Transform::TransformFromMatrix(world_matrix_); }

        virtual void SetWorldTransform(const Transform& transform)
        {
            const auto pref_world_matrix = world_matrix_;
            world_matrix_ = transform.GetMatrix();
            Matrix world_to_local = pref_world_matrix.Invert() * local_matrix_;
            local_matrix_ = world_matrix_ * world_to_local;
            transform_ = Transform::TransformFromMatrix(local_matrix_);
            OnUpdateTransform.Invoke(std::dynamic_pointer_cast<TransformComponent>(shared_from_this()));
        }

        Vector3 GetWorldLocation() const
        {
            return Transform::TransformFromMatrix(world_matrix_).location_;
        }

        virtual void SetWorldLocation(const Vector3& location)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.location_ = location;
            SetWorldTransform(world_trans);
        }

        virtual void AddWorldLocation(const Vector3& location)
        {
            const auto new_location = GetWorldLocation() + location;
            SetWorldLocation(new_location);
        }

        Vector3 GetWorldRotation() const
        {
            return Transform::TransformFromMatrix(world_matrix_).GetRotation();
        }

        Quaternion GetWorldQuatRotation() const
        {
            return Transform::TransformFromMatrix(world_matrix_).rotate_;
        }

        virtual void SetWorldRotation(const Vector3& rotation)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.SetRotation(rotation);
            SetWorldTransform(world_trans);
        }

        virtual void SetWorldRotation(const Quaternion& rotation)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.rotate_ = rotation;
            SetWorldTransform(world_trans);
        }

        virtual void AddWorldRotation(const Vector3& rotation)
        {
            const auto world_trans = Transform::TransformFromMatrix(world_matrix_);
            const auto new_quat = Transform::QuatFromRot(rotation) * world_trans.rotate_;
            SetWorldRotation(new_quat);
        }

        Vector3 GetWorldScale() const
        {
            return Transform::TransformFromMatrix(world_matrix_).scale_;
        }

        virtual void SetWorldScale(const Vector3& scale)
        {
            Transform world_trans = Transform::TransformFromMatrix(world_matrix_);
            world_trans.scale_ = scale;
            SetWorldTransform(world_trans);
        }

        virtual void AddWorldScale(const Vector3& scale)
        {
            const auto new_scale = GetScale() + scale;
            SetWorldScale(new_scale);
        }

        std::weak_ptr<TransformComponent> GetParent() const { return parent_; }

        Matrix GetWorldMatrix() const { return world_matrix_; }
        Matrix GetInverseWorldMatrix() const { return world_matrix_.Invert(); }
        Matrix GetLocalMatrix() const { return local_matrix_; }
        Matrix GetInverseLocalMatrix() const { return local_matrix_.Invert(); }

        void AttachTo(const std::weak_ptr<TransformComponent>& parent)
        {
            if (parent.expired() || parent.lock() == parent_.lock()) return;
            if (!parent_.expired()) Detach();
            parent_ = parent;
            cashed_event_ = parent_.lock()->OnUpdateTransform.Register([this](const std::shared_ptr<TransformComponent>& e) { ParentUpdateTransform(e); });
            UpdateTransformMatrices();
        }

        void Detach()
        {
            if (parent_.expired()) return;
            parent_.lock()->OnUpdateTransform.Unregister(cashed_event_);
            parent_.reset();
            UpdateTransformMatrices();
        }

        EventDispatcher<std::shared_ptr<TransformComponent>> OnUpdateTransform;

        bool attach_translation = true;
        bool attach_rotate = true;
        bool attach_scale = true;

    protected:
        bool is_dirty_ = true;
        Transform transform_;
        Matrix world_matrix_;
        Matrix local_matrix_;
        std::weak_ptr<TransformComponent> parent_;
        EventHandle<std::shared_ptr<TransformComponent>> cashed_event_ = EventHandle<std::shared_ptr<TransformComponent>>::Null();

    private:
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
                is_dirty_ = false;
            }
            OnUpdateTransform.Invoke(std::dynamic_pointer_cast<TransformComponent>(shared_from_this()));
        }

        void CalcWorldTransformMatrix()
        {
            auto rootComp = std::dynamic_pointer_cast<TransformComponent>(shared_from_this());
            world_matrix_ = rootComp->local_matrix_;

            auto parentComp = rootComp->parent_.lock();
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
                parentComp = rootComp->parent_.lock();
            }
        }

        void ParentUpdateTransform(const std::shared_ptr<TransformComponent>& e)
        {
            is_dirty_ = true;
            UpdateTransformMatrices();
        }
    };
#pragma endregion
}
