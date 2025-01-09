module;

#define NOMINMAX
#include <directx/d3dx12.h>
#include <DirectXCollision.h>
#include <unordered_map>
#include <variant>
#include <vector>
#include <directxtk12/SimpleMath.h>

export module PointLightComponent;

import <memory>;
import <json/value.h>;
import <bitset>;
import <filesystem>;
import <algorithm>;
import <optional>;

import Engine;
import Component;
import MeshAsset;
import Material;
import IRenderable;
import SceneVisibility;
import TransformComponent;
import EventSystem;
import GameObject;
import Misc;
import IObjectShaderResource;
import PerObjectData;
import StubTexturesHandles;
import IUpdateGPUData;
import ViewTypes;
import BufferView;
import GPULocalResource;
import PrefabInstance;
import LightComponent;

namespace GiiGa
{
    struct alignas(256) PointLightData
    {
        alignas(16) DirectX::SimpleMath::Vector3 posWS;
        alignas(16) DirectX::SimpleMath::Vector3 color = {1, 0, 0};
        float radius = 1;
        float max_intensity = 1;
        float falloff = 1;
    };

    export class PointLightShaderResource : public IObjectShaderResource
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

    export class PointLightComponent : public LightComponent
    {
    public:
        PointLightComponent():
            pointLightShaderRes_(std::make_shared<PointLightShaderResource>())
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

        ~PointLightComponent()
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

        std::vector<std::pair<PropertyModificationKey,PropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const PropertyModifications& modifications) override
        {
            Todo();
        }


        void Draw(RenderContext& context) override
        {
            mesh_->Draw(context.GetGraphicsCommandList());
        }

        SortData GetSortData() override
        {
            return {.object_mask = mesh_->GetObjectMask().SetFillMode(FillMode::Wire).SetLightType(LightType::Point), .shaderResource = pointLightShaderRes_};
        }

        void Restore(const Json::Value&) override
        {
            Todo();
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Todo();
            return {};
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

        PerObjectData& GetPerObjectData() override
        {
            return *perObjectData_;
        }

        void SetColor(const DirectX::SimpleMath::Vector3& color) override
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

        void SetIntensity(float max_intensity) override
        {
            data_.max_intensity = max_intensity;
            isDirty = true;
        }

        void SetFallOff(float falloff)
        {
            data_.falloff = falloff;
            isDirty = true;
        }

        PointLightData GetData() const {return data_;}

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
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
                [this](const UpdateTransformEvent& e)
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
