#pragma once
#include <pybind11/conduit/wrap_include_python_h.h>

#include <directx/d3dx12.h>
#include <DirectXCollision.h>
#include <unordered_map>
#include <vector>
#include <directxtk12/SimpleMath.h>

#include<memory>
#include<json/value.h>
#include<algorithm>
#include<optional>

#include<Engine.h>
#include<Component.h>
#include<ConcreteAsset/MeshAsset.h>
#include<IRenderable.h>
#include<SceneVisibility.h>
#include<TransformComponent.h>
#include<EventSystem.h>
#include<GameObject.h>
#include<Misc.h>
#include<IObjectShaderResource.h>
#include<PerObjectData.h>
#include<DefaultAssetsHandles.h>
#include<IUpdateGPUData.h>
#include<ViewTypes.h>
#include<BufferView.h>
#include<GPULocalResource.h>
#include<PrefabInstanceModifications.h>
#include<LightComponent.h>

namespace GiiGa
{
    struct alignas(256) PointLightData
    {
        alignas(16) DirectX::SimpleMath::Vector3 posWS;
        alignas(16) DirectX::SimpleMath::Vector3 color = {1, 0, 0};
        float radius = 1;
        float max_intensity = 1;
        float falloff = 1;

        PointLightData() = default;

        PointLightData(const Json::Value& json)
        {
            max_intensity = json["max_intensity"].asFloat();
            color = Vector3FromJson(json["color"]);
            falloff = json["falloff"].asFloat();
        }

        Json::Value toJson()
        {
            Json::Value json;

            json["max_intensity"] = max_intensity;
            json["color"] = Vector3ToJson(color);
            json["falloff"] = falloff;

            return json;
        }
    };

    class PointLightShaderResource : public IObjectShaderResource
    {
        friend class PointLightComponent;

    public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result(1);
            result[0] = PointLightCBV_->getDescriptor().getGPUHandle();
            return result;
        }

    private:
        std::shared_ptr<BufferView<Constant>> PointLightCBV_;
    };

    class PointLightComponent : public LightComponent
    {
    public:
        PointLightComponent(const Json::Value& json, bool roll_id = false):
            LightComponent(json, roll_id),
            pointLightShaderRes_(std::make_shared<PointLightShaderResource>()),
            data_(json["PointLightData"])
        {
            ConstructFunction();
        }

        PointLightComponent():
            pointLightShaderRes_(std::make_shared<PointLightShaderResource>())
        {
            ConstructFunction();
        }

        void ConstructFunction()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();
            auto& context = Engine::Instance().RenderSystem()->GetRenderContext();

            if (isStatic_)
            {
                UINT SizeInBytes = sizeof(PointLightData);

                const auto span = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};
                poinLightRes = std::make_unique<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
                poinLightRes->UpdateContentsDeffered(context, span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(poinLightRes->GetResource()->GetGPUVirtualAddress(), SizeInBytes);
                pointLightShaderRes_->PointLightCBV_ = poinLightRes->CreateConstantBufferView(desc);
            }
        }

        ~PointLightComponent() override
        {
            Engine::Instance().RenderSystem()->UnregisterInUpdateGPUData(this);
            if (auto l_owner = owner_.lock())
            {
                if (auto l_trans = std::dynamic_pointer_cast<GameObject>(l_owner)->GetTransformComponent().lock())
                    l_trans->OnUpdateTransform.Unregister(cashed_event_);
            }
        }

        void Tick(float dt) override
        {
        }

        void Init() override
        {
            transform_ = std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent();
            if (!mesh_)
            {
                auto rm = Engine::Instance().ResourceManager();

                mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(DefaultAssetsHandles::Sphere);

                RegisterInVisibility();
            }
        }

        void BeginPlay() override
        {
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>
                                          & instance_uuid) override
        {
            Todo();
            return {};
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            Todo();
        }


        void Draw(RenderContext& context) override
        {
            mesh_->Draw(context.GetGraphicsCommandList());
        }

        SortData GetSortData() override
        {
            return {
                .object_mask = mesh_->GetObjectMask()
                                    .SetLightType(LightType::Point),
                .shaderResource = pointLightShaderRes_
            };
        }

        void RestoreFromLevelJson(const Json::Value&) override
        {
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            result["Type"] = typeid(PointLightComponent).name();
            result["PointLightData"] = data_.toJson();;
            return result;
        }

        void UpdateGPUData(RenderContext& context) override
        {
            if (!isDirty)
                return;

            if (!perObjectData_)
                perObjectData_ = std::make_shared<PerObjectData>(context, transform_.lock(), isStatic_);
            perObjectData_->UpdateGPUData(context);

            UINT SizeInBytes = sizeof(PointLightData);

            const auto span = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};

            if (isStatic_)
            {
                poinLightRes->UpdateContentsImmediate(context, span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
                pointLightShaderRes_->PointLightCBV_ = context.AllocateDynamicConstantView(span, desc);
            }
        }

        void InitShadowData(RenderDevice& device) override
        {
        };

        PerObjectData& GetPerObjectData() override
        {
            return *perObjectData_;
        }

        void SetColor(const DirectX::SimpleMath::Vector3& color)
        {
            data_.color = color;
            isDirty = true;
        }

        void SetRadius(float radius)
        {
            data_.radius = radius;
            std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent().lock()->SetScale({radius, radius, radius});
            isDirty = true;
        }

        void SetIntensity(float max_intensity)
        {
            data_.max_intensity = max_intensity;
            isDirty = true;
        }

        void SetFallOff(float falloff)
        {
            data_.falloff = falloff;
            isDirty = true;
        }

        PointLightData GetData() const { return data_; }

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<std::shared_ptr<TransformComponent>> cashed_event_ = EventHandle<std::shared_ptr<TransformComponent>>::Null();
        std::shared_ptr<PerObjectData> perObjectData_;
        std::unique_ptr<GPULocalResource> poinLightRes;
        std::shared_ptr<PointLightShaderResource> pointLightShaderRes_;
        PointLightData data_;

        bool isStatic_ = false;
        bool isDirty = true;

        void RegisterInVisibility()
        {
            visibilityEntry_ = VisibilityEntry::Register(std::dynamic_pointer_cast<IRenderable>(shared_from_this()), mesh_->GetAABB());
            if (cashed_event_.isValid())
            {
                std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent().lock()->OnUpdateTransform.Unregister(cashed_event_);
            }
            cashed_event_ = std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent().lock()->OnUpdateTransform.Register(
                [this](const std::shared_ptr<TransformComponent>& e)
                {
                    auto owner_go = std::dynamic_pointer_cast<GameObject>(owner_.lock());
                    Transform trans = owner_go->GetComponent<TransformComponent>()->GetWorldTransform();
                    DirectX::BoundingBox origaabb = mesh_->GetAABB();

                    origaabb.Transform(origaabb, trans.GetMatrix());

                    data_.posWS = trans.location_;
                    data_.radius = std::max(trans.scale_.x, std::max(trans.scale_.y, trans.scale_.z));
                    isDirty = true;

                    visibilityEntry_->Update(origaabb);
                });
        }
    };
}
