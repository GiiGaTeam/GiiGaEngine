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


namespace GiiGa
{
    struct alignas(256) ColorData
    {
        DirectX::SimpleMath::Vector3 color = {0.5f, 0.0f, 0.5f};
    };

    class CollisionShaderResource : public IObjectShaderResource
    {
    public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result(1);
            result[0] = colorCBV_->getDescriptor().getGPUHandle();
            return result;
        }

        std::shared_ptr<BufferView<Constant>> colorCBV_;
    };

    class CollisionComponent : public IRenderable, public IUpdateGPUData, public ICollision
    {
    public:
        CollisionComponent()
            : colorShaderRes_(std::make_shared<CollisionShaderResource>())
        {
            collider_type_ = ColliderType::Cube;
            motion_type_ = EMotionType::Static;
            ConstructFunction();
        }

        CollisionComponent(const Json::Value& json, bool roll_id = false) : ICollision(json, roll_id),
                                                                            colorShaderRes_(std::make_shared<CollisionShaderResource>())
        {
            motion_type_ = static_cast<EMotionType>(json["MotionType"].asInt());
            collider_type_ = static_cast<ColliderType>(json["ColliderType"].asInt());
            layer_ = static_cast<Layer>(json["Layer"].asInt());
            is_gravity_active_ = static_cast<Layer>(json["ActiveGravity"].asBool());
            ConstructFunction();
        }

        ~CollisionComponent() override
        {
            Engine::Instance().RenderSystem()->UnregisterInUpdateGPUData(this);
            PhysicsSystem::UnRegisterCollision(this);
            OnUpdateTransform.Unregister(cashed_event_);
        }

        void ConstructFunction()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result = TransformComponent::DerivedToJson(is_prefab_root);
            result["Type"] = typeid(CollisionComponent).name();
            result["ColliderType"] = collider_type_;
            result["MotionType"] = static_cast<int32_t>(motion_type_);
            result["Layer"] = layer_;
            result["ActiveGravity"] = is_gravity_active_;

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
            switch (collider_type_)
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
        }

        void Init() override
        {
            AttachTo(std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent());

            if (!mesh_)
            {
                ChooseMeshAsset();

                RegisterInVisibility();
            }
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

        void Draw(RenderContext& context) override
        {
            mesh_->Draw(context.GetGraphicsCommandList());
        }

        SortData GetSortData() override
        {
            return {
                .object_mask = mesh_->GetObjectMask()
                                    .SetFillMode(FillMode::Wire)
                                    .SetBlendMode(BlendMode::Debug),
                .shaderResource = colorShaderRes_
            };
        }

        PerObjectData& GetPerObjectData() override
        {
            return *perObjectData_;
        }

        void UpdateGPUData(RenderContext& context) override
        {
            if (!perObjectData_)
                perObjectData_ = std::make_shared<PerObjectData>(context, std::dynamic_pointer_cast<CollisionComponent>(shared_from_this()), motion_type_ == EMotionType::Static);
            perObjectData_->UpdateGPUData(context);

            UINT SizeInBytes = sizeof(ColorData);

            ColorData colorData;

            const auto span = std::span{reinterpret_cast<uint8_t*>(&colorData), SizeInBytes};

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
            colorShaderRes_->colorCBV_ = context.AllocateDynamicConstantView(span, desc);
        }

        void SetColliderType(ColliderType new_type) override
        {
            OnUpdateType.Invoke(new_type);
            collider_type_ = new_type;
            ChooseMeshAsset();
        }

        ColliderType GetColliderType() const override { return collider_type_; }

        EMotionType GetMotionType() const override { return motion_type_; }

        void SetMotionType(EMotionType motion_type) override { motion_type_ = motion_type; }

        Layer GetLayer() const override { return layer_; }

        void SetLayer(Layer layer) override { layer_ = layer; }

        bool IsGravityActive() const override { return is_gravity_active_; }

        void SetGravityActive(bool gravity) override { is_gravity_active_ = gravity; }

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

        bool canCollideSelf_ = false;

    protected:
        ColliderType collider_type_;
        EMotionType motion_type_;
        Layer layer_;
        bool is_gravity_active_;

        std::unique_ptr<GPULocalResource> colorRes_;
        std::shared_ptr<CollisionShaderResource> colorShaderRes_;

    private:
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        std::shared_ptr<PerObjectData> perObjectData_;
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        EventHandle<std::shared_ptr<TransformComponent>> cashed_event_ = EventHandle<std::shared_ptr<TransformComponent>>::Null();

        JPH::BodyID body_id_;

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
    };
}
