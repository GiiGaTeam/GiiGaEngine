#pragma once
#include "DefaultAssetsHandles.h"
#include "IUpdateGPUData.h"
#include "IRenderable.h"
#include "TransformComponent.h"
#include "ConcreteAsset/MeshAsset.h"
#include <json/value.h>
#include "SceneVisibility.h"
#include "Engine.h"
#include "ICollision.h"
#include "Physics/PhysicsSystem.h"

#include <nvrhi/nvrhi.h>
#include <Core/Render/Nvrhi/NvrhiResources.h>


namespace GiiGa
{
    struct alignas(256) ColorData
    {
        DirectX::SimpleMath::Vector3 color = {0.5f, 0.0f, 0.5f};
    };

    // NVRHI cutover: exposes the collision debug color constant buffer via IObjectShaderResource.
    // The Debug pass binds bindings_.constantBuffer when drawing the wireframe collision mesh.
    class CollisionShaderResource : public IObjectShaderResource
    {
        friend class CollisionComponent;

    public:
        const MaterialBindings& GetMaterialBindings() override { return bindings_; }

    private:
        nvrhi::BufferHandle cb_;
        MaterialBindings bindings_;
    };

    class CollisionComponent : public IRenderable, public IUpdateGPUData, public ICollision
    {
    public:
        CollisionComponent()
            : colorShaderRes_(std::make_shared<CollisionShaderResource>())
        {
            collisionData_.collider_type_ = ColliderType::Cube;
            collisionData_.motion_type_ = EMotionType::Static;
            ConstructFunction();
        }

        CollisionComponent(const Json::Value& json, bool roll_id = false) : ICollision(json, roll_id),
                                                                            colorShaderRes_(std::make_shared<CollisionShaderResource>())
        {
            collisionData_.motion_type_ = static_cast<EMotionType>(json["MotionType"].asInt());
            collisionData_.collider_type_ = static_cast<ColliderType>(json["ColliderType"].asInt());
            collisionData_.layer_ = static_cast<Layer>(json["Layer"].asInt());

            collisionData_.friction_ = !json["Friction"].empty() ? json["Friction"].asFloat() : 0.2f;
            collisionData_.restitution_ = !json["Restitution"].empty() ? json["Restitution"].asFloat() : 0.0f;
            collisionData_.gravityFactor_ = !json["GravityFactor"].empty() ? json["GravityFactor"].asFloat() : 1.0f;

            ConstructFunction();
        }

        ~CollisionComponent() override
        {
            if (auto rs = Engine::Instance().RenderSystem())
                rs->UnregisterInUpdateGPUData(this);
            PhysicsSystem::UnRegisterCollision(this);
            OnUpdateTransform.Unregister(cashed_event_);
        }

        void ConstructFunction()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);
            if (auto dev = Engine::Instance().RenderSystem()->GetNvrhiDevice())
                EnsureColorBuffer_(dev);
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result = TransformComponent::DerivedToJson(is_prefab_root);
            result["Type"] = typeid(CollisionComponent).name();
            result["ColliderType"] = GetColliderType();
            result["MotionType"] = static_cast<int32_t>(GetMotionType());
            result["Layer"] = GetLayer();

            result["Friction"] = collisionData_.friction_;
            result["Restitution"] = collisionData_.restitution_;
            result["GravityFactor"] = collisionData_.gravityFactor_;

            return result;
        }

        void ChooseMeshAsset()
        {
            if (mesh_)
            {
                visibilityEntry_->Unregister();
            }

            auto rm = Engine::Instance().ResourceManager();

            AssetHandle mesh_asset;
            switch (collisionData_.collider_type_)
            {
            case Cube:
                mesh_asset = DefaultAssetsHandles::Cube;
                break;
            case Sphere:
                mesh_asset = DefaultAssetsHandles::Sphere;
                break;
            }
            mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(mesh_asset);
            visibilityEntry_ = VisibilityEntry::Register(std::dynamic_pointer_cast<CollisionComponent>(shared_from_this()), mesh_->GetAABB());
            RefreshCached();
        }

        void Init() override
        {
            AttachTo(std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent());

            if (!mesh_)
            {
                ChooseMeshAsset();

                RegisterInVisibility();
            }
            perObjectData_.SetTransform(std::dynamic_pointer_cast<TransformComponent>(shared_from_this()));
        }

        void SetOwnerWorldLocation(DirectX::SimpleMath::Vector3 loc) override
        {
            if (!parent_.expired()) parent_.lock()->SetWorldLocation(loc);
        };

        void SetOwnerWorldRotation(DirectX::SimpleMath::Quaternion quat) override
        {
            if (!parent_.expired()) parent_.lock()->SetWorldRotation(quat);
        };

        void BeginPlay() override
        {
            TransformComponent::BeginPlay();

            RegisterInPhysics();
        }

        void Tick(float dt) override
        {
        }

        // --- IRenderable: debug wireframe geometry + color data provider ---

        nvrhi::IBuffer* GetVertexBuffer() override { return mesh_ ? mesh_->GetVertexBuffer() : nullptr; }
        nvrhi::IBuffer* GetIndexBuffer() override { return mesh_ ? mesh_->GetIndexBuffer() : nullptr; }
        uint32_t GetIndexCount() override { return mesh_ ? mesh_->GetIndexCount() : 0; }
        nvrhi::Format GetIndexFormat() override { return nvrhi::Format::R16_UINT; }

        PerObjectData& GetPerObjectData() override { return perObjectData_; }
        std::shared_ptr<IObjectShaderResource> GetShaderResource() override { return colorShaderRes_; }

        const ObjectMask& GetObjectMask() override { return objectMask_; }
        const DirectX::BoundingBox& GetAABB() override { return aabb_; }

        // --- IUpdateGPUData: push color data to the NVRHI constant buffer when dirty ---
        void UpdateGPUData(nvrhi::IDevice* device, nvrhi::ICommandList* cmd) override
        {
            if (!isDirty)
                return;
            isDirty = false;

            EnsureColorBuffer_(device);

            if (colorShaderRes_->cb_ && cmd)
            {
                ColorData colorData;
                cmd->writeBuffer(colorShaderRes_->cb_, &colorData, sizeof(ColorData));
            }
        }

        void SetColliderType(ColliderType new_type) override
        {
            OnUpdateType.Invoke(new_type);
            collisionData_.collider_type_ = new_type;
            ChooseMeshAsset();
        }

        ColliderType GetColliderType() const override { return collisionData_.collider_type_; }

        EMotionType GetMotionType() const override { return collisionData_.motion_type_; }

        void SetMotionType(EMotionType motion_type) override
        {
            collisionData_.motion_type_ = motion_type;
            UpdateData();
        }

        Layer GetLayer() const override { return collisionData_.layer_; }

        void SetLayer(Layer layer) override
        {
            collisionData_.layer_ = layer;
            UpdateData();
        }

        CollisionData GetData() const override { return collisionData_; }

        void SetData(CollisionData data) override
        {
            collisionData_ = data;
            UpdateData();
        }

        void OnContactAdded(const std::shared_ptr<ICollision>& other_comp, const CollideInfo& collideInfo) override
        {
            if (!other_comp || other_comp == shared_from_this()) return;
            auto col_comp = std::dynamic_pointer_cast<CollisionComponent>(other_comp);
            if (col_comp->GetOwner() == GetOwner() && !canCollideSelf_) return;

            GetOwner()->OnBeginOverlap(col_comp, collideInfo);
        }

        void OnContactPersisted(const std::shared_ptr<ICollision>& other_comp, const CollideInfo& collideInfo) override
        {
            if (!other_comp || other_comp == shared_from_this()) return;
            auto col_comp = std::dynamic_pointer_cast<CollisionComponent>(other_comp);
            if (col_comp->GetOwner() == GetOwner() && !canCollideSelf_) return;

            GetOwner()->OnOverlapping(col_comp, collideInfo);
        }

        void OnContactRemoved(const std::shared_ptr<ICollision>& other_comp) override
        {
            if (!other_comp || other_comp == shared_from_this()) return;
            auto col_comp = std::dynamic_pointer_cast<CollisionComponent>(other_comp);
            if (col_comp->GetOwner() == GetOwner() && !canCollideSelf_) return;

            GetOwner()->OnEndOverlap(col_comp);
        }

        void AddForce(const DirectX::SimpleMath::Vector3& force) override
        {
            if (!body_id_.IsInvalid())
                PhysicsSystem::GetBodyInterface().AddForce(body_id_, VecToJoltVec(force));
        }

        void AddImpulse(const DirectX::SimpleMath::Vector3& impulse) override
        {
            if (!body_id_.IsInvalid())
                PhysicsSystem::GetBodyInterface().AddImpulse(body_id_, VecToJoltVec(impulse));
        }

        void AddVelocity(const DirectX::SimpleMath::Vector3& velocity) override
        {
            if (!body_id_.IsInvalid())
                PhysicsSystem::GetBodyInterface().AddLinearVelocity(body_id_, VecToJoltVec(velocity));
        }

        void AddLinearAndAngularVelocity(const DirectX::SimpleMath::Vector3& velocity, const DirectX::SimpleMath::Vector3& angular) override
        {
            if (!body_id_.IsInvalid())
                PhysicsSystem::GetBodyInterface().AddLinearAndAngularVelocity(body_id_, VecToJoltVec(velocity), VecToJoltVec(angular));
        }

        void SetVelocity(const DirectX::SimpleMath::Vector3& velocity) override
        {
            if (!body_id_.IsInvalid())
                PhysicsSystem::GetBodyInterface().SetLinearVelocity(body_id_, VecToJoltVec(velocity));
        }

        void SetAngularVelocity(const DirectX::SimpleMath::Vector3& angular) override
        {
            if (!body_id_.IsInvalid())
                PhysicsSystem::GetBodyInterface().SetAngularVelocity(body_id_, VecToJoltVec(angular));
        }

        bool canCollideSelf_ = false;

    protected:
        CollisionData collisionData_;

        std::shared_ptr<CollisionShaderResource> colorShaderRes_;

    private:
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        PerObjectData perObjectData_;
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        EventHandle<std::shared_ptr<TransformComponent>> cashed_event_ = EventHandle<std::shared_ptr<TransformComponent>>::Null();

        ObjectMask objectMask_;
        DirectX::BoundingBox aabb_;
        bool isDirty = true;

        JPH::BodyID body_id_;

        void EnsureColorBuffer_(nvrhi::IDevice* device)
        {
            if (!colorShaderRes_->cb_ && device)
            {
                colorShaderRes_->cb_ = rhi::createConstantBuffer(device, sizeof(ColorData), "collision color cb");
                colorShaderRes_->bindings_.constantBuffer = colorShaderRes_->cb_.Get();
            }
        }

        void RefreshCached()
        {
            if (mesh_)
            {
                objectMask_ = mesh_->GetObjectMask().SetFillMode(FillMode::Wire).SetBlendMode(BlendMode::Debug);
                aabb_ = mesh_->GetAABB();
            }
        }

        void RegisterInVisibility()
        {
            if (cashed_event_.isValid())
            {
                OnUpdateTransform.Unregister(cashed_event_);
            }
            cashed_event_ = OnUpdateTransform.Register(
                [this](const std::shared_ptr<TransformComponent>& e)
                {
                    DirectX::BoundingBox origaabb = mesh_->GetAABB();

                    origaabb.Transform(origaabb, GetWorldMatrix());

                    visibilityEntry_->Update(origaabb);
                });
        }

        void RegisterInPhysics()
        {
            body_id_ = PhysicsSystem::RegisterCollision(std::dynamic_pointer_cast<CollisionComponent>(shared_from_this()));
        }

        void UpdateData()
        {
            if (!body_id_.IsInvalid())
            {
                PhysicsSystem::GetBodyInterface().SetFriction(body_id_, collisionData_.friction_);
                PhysicsSystem::GetBodyInterface().SetRestitution(body_id_, collisionData_.restitution_);
                PhysicsSystem::GetBodyInterface().SetGravityFactor(body_id_, collisionData_.gravityFactor_);
                PhysicsSystem::GetBodyInterface().SetMotionType(body_id_, static_cast<JPH::EMotionType>(collisionData_.motion_type_), JPH::EActivation::Activate);
                PhysicsSystem::GetBodyInterface().SetObjectLayer(body_id_, collisionData_.layer_);
            }
        };
    };
}
