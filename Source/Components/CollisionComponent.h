#pragma once
/*#include "DefaultAssetsHandles.h"
#include "GameObject.h"
#include "Assets/ConcreteAsset/MeshAsset.h"
#include "SceneVisibility.h"
#include "TransformComponent.h"
#include "VertexTypes.h"*/
#include "DefaultAssetsHandles.h"
#include "Engine.h"
#include "IUpdateGPUData.h"
#include "IRenderable.h"
#include "SceneVisibility.h"
#include "TransformComponent.h"
#include "ConcreteAsset/MeshAsset.h"

namespace JPH
{
    enum class EMotionType : uint8;
}

namespace GiiGa
{
    enum ColliderType
    {
        Box = 0,
        Sphere = 1,
    };

    class CollisionComponent : public TransformComponent, public IRenderable, public IUpdateGPUData
    {
    public:
        CollisionComponent() = default;

        CollisionComponent(const Json::Value& json, bool roll_id = false);

        Json::Value DerivedToJson(bool is_prefab_root) override
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

        void Init() override
        {
            attach_rotate = false;
            SetWorldRotation(Vector3::Zero);

            AttachTo(std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent());

            if (!mesh_)
            {
                auto rm = Engine::Instance().ResourceManager();

                AssetHandle mesh_asset;
                switch (collider_type_)
                {
                case Box:
                    mesh_asset = DefaultAssetsHandles::Cube;
                    break;
                case Sphere:
                    mesh_asset = DefaultAssetsHandles::Sphere;
                    break;
                }
                mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(mesh_asset);

                RegisterInVisibility();
            }

            RegisterInPhysics();
        }

        void BeginPlay() override
        {
            TransformComponent::BeginPlay();
            RegisterInPhysics();
        }

        void Tick(float dt) override;

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
                .shaderResource = nullptr
            };
        }

        PerObjectData& GetPerObjectData() override
        {
            return *perObjectData_;
        }

        void UpdateGPUData(RenderContext& context) override;

        void SetColliderType(ColliderType new_type)
        {
            OnUpdateType.Invoke(new_type);
            collider_type_ = new_type;
        }

        ColliderType GetColliderType() const { return collider_type_; }

        JPH::EMotionType GetMotionType() const { return motion_type_; }

        void SetMotionType(JPH::EMotionType motion_type) { motion_type_ = motion_type; }

        EventDispatcher<ColliderType> OnUpdateType;

    protected:
        ColliderType collider_type_;
        JPH::EMotionType motion_type_;

        virtual void RegisterInPhysics();

    private:
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        std::shared_ptr<PerObjectData> perObjectData_;
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();

        void RegisterInVisibility()
        {
            visibilityEntry_.reset();
            visibilityEntry_ = VisibilityEntry::Register(std::dynamic_pointer_cast<CollisionComponent>(shared_from_this()), mesh_->GetAABB());
            if (cashed_event_.isValid())
            {
                OnUpdateTransform.Unregister(cashed_event_);
            }
            cashed_event_ = OnUpdateTransform.Register(
                [this](const UpdateTransformEvent& e)
                {
                    auto owner_go = std::dynamic_pointer_cast<GameObject>(owner_.lock());
                    Transform trans = owner_go->GetComponent<TransformComponent>()->GetWorldTransform();
                    DirectX::BoundingBox origaabb = mesh_->GetAABB();

                    origaabb.Transform(origaabb, trans.GetMatrix());

                    visibilityEntry_->Update(origaabb);
                });
        }
    };
}
