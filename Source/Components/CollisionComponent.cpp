#include "CollisionComponent.h"
#include "DefaultAssetsHandles.h"
#include "GameObject.h"
#include "IRenderable.h"
#include "IUpdateGPUData.h"
#include "Assets/ConcreteAsset/MeshAsset.h"
#include "SceneVisibility.h"
#include "TransformComponent.h"
#include "VertexTypes.h"
#include "Physics/PhysicsSystem.h"

namespace GiiGa
{
    CollisionComponent::CollisionComponent(const Json::Value& json, bool roll_id) :TransformComponent(json, roll_id)
    {
        motion_type_ = JPH::EMotionType::Static;
        collider_type_ = static_cast<ColliderType>(json["ColliderType"].asInt());
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
