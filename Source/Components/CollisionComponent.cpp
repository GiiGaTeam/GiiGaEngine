#include "Components/CollisionComponent.h"
#include "Assets/ConcreteAsset/MeshAsset.h"
#include "TransformComponent.h"
#include "Physics/PhysicsSystem.h"
#include<json/value.h>

namespace GiiGa
{
    CollisionComponent::CollisionComponent()
    {
        collider_type_ = ColliderType::Box;
        motion_type_ = JPH::EMotionType::Static;
    }

    CollisionComponent::CollisionComponent(const Json::Value& json, bool roll_id)
    {
        motion_type_ = JPH::EMotionType::Static;
        collider_type_ = static_cast<ColliderType>(json["ColliderType"].asInt());
    }


    Json::Value CollisionComponent::DerivedToJson(bool is_prefab_root)
    {
        Json::Value result;

        result["Type"] = typeid(CollisionComponent).name();

        if (!is_prefab_root)
            result["Transform"] = transform_.ToJson();
        else
            result["Transform"] = Transform{}.ToJson();

        result["ColliderType"] = collider_type_;

        return result;
    }

    inline void CollisionComponent::Tick(float dt)
    {
    }


    void CollisionComponent::UpdateGPUData(RenderContext& context)
    {
        if (!perObjectData_)
            perObjectData_ = std::make_shared<PerObjectData>(context, std::dynamic_pointer_cast<TransformComponent>(shared_from_this()), motion_type_ == JPH::EMotionType::Static);
        perObjectData_->UpdateGPUData(context);
    }

    void CollisionComponent::RegisterInPhysics()
    {
        PhysicsSystem::RegisterCollision(std::dynamic_pointer_cast<CollisionComponent>(shared_from_this()));
    }
}
