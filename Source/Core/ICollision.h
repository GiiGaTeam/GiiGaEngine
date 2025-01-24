#pragma once
#include<memory>
#include<list>
#include<vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include <DirectXCollision.h>

#include<unique_any.h>
#include <directxtk12/SimpleMath.h>
#include <TransformComponent.h>

namespace GiiGa
{
    struct CollideInfo
    {
        Vector3 baseOffset;
        Vector3 normal;
        float depthPenetration;
    };

    enum class EMotionType
    {
        Static,    ///< Non movable
        Kinematic, ///< Movable using velocities only, does not respond to forces
        Dynamic,   ///< Responds to forces as a normal physics object
    };

    enum ColliderType
    {
        Cube = 0,
        Sphere = 1,
    };

    enum Layer
    {
        NoMoving = 0,
        Moving = 1,
        Trigger = 2,
    };


    struct ICollision : public TransformComponent
    {
    public:
        ICollision() = default;

        ICollision(const Json::Value& json, bool roll_id = false) : TransformComponent(json, roll_id)
        {
        }

        virtual void OnContactAdded(const std::shared_ptr<ICollision>& other_comp, const CollideInfo& collideInfo) =0;
        virtual void OnContactPersisted(const std::shared_ptr<ICollision>& other_comp, const CollideInfo& collideInfo) =0;
        virtual void OnContactRemoved(const std::shared_ptr<ICollision>& other_comp) =0;
        virtual void SetOwnerWorldLocation(DirectX::SimpleMath::Vector3 loc) =0;
        virtual void SetOwnerWorldRotation(DirectX::SimpleMath::Quaternion quat) =0;

        virtual void AddForce(const DirectX::SimpleMath::Vector3& force) = 0;
        virtual void AddImpulse(const DirectX::SimpleMath::Vector3& impulse) = 0;
        virtual void AddVelocity(const DirectX::SimpleMath::Vector3& velocity) = 0;

        virtual void SetColliderType(ColliderType new_type) =0;
        virtual ColliderType GetColliderType() const =0;

        virtual EMotionType GetMotionType() const =0;
        virtual void SetMotionType(EMotionType motion_type) =0;

        virtual Layer GetLayer() const =0;
        virtual void SetLayer(Layer layer) =0;

        virtual bool IsGravityActive() const =0;
        virtual void SetGravityActive(bool gravity) =0;

        EventDispatcher<ColliderType> OnUpdateType;
        EventDispatcher<EMotionType> OnUpdateMotion;
        EventDispatcher<ColliderType> OnAddForce;
        EventDispatcher<ColliderType> OnAddImpulse;
        EventDispatcher<ColliderType> OnAddVelocity;
    };
}
