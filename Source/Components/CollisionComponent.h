#pragma once
#include "DefaultAssetsHandles.h"
#include "IUpdateGPUData.h"
#include "IRenderable.h"
#include "TransformComponent.h"
#include "ConcreteAsset/MeshAsset.h"
#include <json/value.h>
#include "SceneVisibility.h"
#include "Engine.h"


namespace GiiGa
{
    struct alignas(256) ColorData
    {
        DirectX::SimpleMath::Vector3 color = {0.5f, 0.0f, 0.5f};
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

    class CollisionComponent : public TransformComponent, public IRenderable, public IUpdateGPUData
    {
    public:
        CollisionComponent()
            : colorShaderRes_(std::make_shared<CollisionShaderResource>())
        {
            collider_type_ = ColliderType::Cube;
            motion_type_ = EMotionType::Static;
            ConstructFunction();
        }

        CollisionComponent(const Json::Value& json, bool roll_id = false) : TransformComponent(json, roll_id),
                                                                            colorShaderRes_(std::make_shared<CollisionShaderResource>())
        {
            motion_type_ = static_cast<EMotionType>(json["MotionType"].asInt());
            collider_type_ = static_cast<ColliderType>(json["ColliderType"].asInt());
            ConstructFunction();
        }

        void ConstructFunction()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;

            result["Type"] = typeid(CollisionComponent).name();

            if (!is_prefab_root)
                result["Transform"] = transform_.ToJson();
            else
                result["Transform"] = Transform{}.ToJson();

            result["ColliderType"] = collider_type_;
            result["MotionType"] = static_cast<int32_t>(motion_type_);

            return result;
        }

        void Init() override
        {
            AttachTo(std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent());

            if (!mesh_)
            {
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

                RegisterInVisibility();
            }
        }

        void SetOwnerWorldLocation(DirectX::SimpleMath::Vector3 loc)
        {
            if (!parent_.expired()) parent_.lock()->SetWorldLocation(loc);
        };

        void SetOwnerWorldRotation(DirectX::SimpleMath::Quaternion quat)
        {
            if (!parent_.expired()) parent_.lock()->SetWorldRotation(quat);
        };

        void BeginPlay() override
        {
            TransformComponent::BeginPlay();
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
                perObjectData_ = std::make_shared<PerObjectData>(context, std::dynamic_pointer_cast<TransformComponent>(shared_from_this()), motion_type_ == EMotionType::Static);
            perObjectData_->UpdateGPUData(context);

            UINT SizeInBytes = sizeof(ColorData);

            ColorData colorData;

            const auto span = std::span{reinterpret_cast<uint8_t*>(&colorData), SizeInBytes};

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
            colorShaderRes_->colorCBV_ = context.AllocateDynamicConstantView(span, desc);
        }

        void SetColliderType(ColliderType new_type)
        {
            OnUpdateType.Invoke(new_type);
            collider_type_ = new_type;
        }

        ColliderType GetColliderType() const { return collider_type_; }

        EMotionType GetMotionType() const { return motion_type_; }

        void SetMotionType(EMotionType motion_type) { motion_type_ = motion_type; }

        EventDispatcher<ColliderType> OnUpdateType;

    protected:
        ColliderType collider_type_;
        EMotionType motion_type_;

        std::unique_ptr<GPULocalResource> colorRes_;
        std::shared_ptr<CollisionShaderResource> colorShaderRes_;

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
