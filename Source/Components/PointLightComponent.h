#pragma once
#include <pybind11/conduit/wrap_include_python_h.h>

#include <DirectXCollision.h>
#include <unordered_map>
#include <vector>
#include <directxtk12/SimpleMath.h>

#include<memory>
#include<json/value.h>
#include<algorithm>
#include<optional>

#include<nvrhi/nvrhi.h>

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
#include<PrefabInstanceModifications.h>
#include<LightComponent.h>
#include<Core/Render/Nvrhi/NvrhiResources.h>
#include<DXMathUtils.h>

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

    // NVRHI cutover: exposes the point-light constant buffer through IObjectShaderResource.
    // The GLight pass binds bindings_.constantBuffer (the light data) when drawing the light mesh.
    class PointLightShaderResource : public IObjectShaderResource
    {
        friend class PointLightComponent;

    public:
        const MaterialBindings& GetMaterialBindings() override { return bindings_; }

    private:
        nvrhi::BufferHandle cb_;
        MaterialBindings bindings_;
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

            if (auto dev = Engine::Instance().RenderSystem()->GetNvrhiDevice())
            {
                EnsureLightBuffer_(dev);
            }
        }

        ~PointLightComponent() override
        {
            if (auto rs = Engine::Instance().RenderSystem())
                rs->UnregisterInUpdateGPUData(this);
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
            perObjectData_.SetTransform(transform_.lock());
            RefreshCached();
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

        // --- IRenderable: light geometry + data provider ---

        nvrhi::IBuffer* GetVertexBuffer() override { return mesh_ ? mesh_->GetVertexBuffer() : nullptr; }
        nvrhi::IBuffer* GetIndexBuffer() override { return mesh_ ? mesh_->GetIndexBuffer() : nullptr; }
        uint32_t GetIndexCount() override { return mesh_ ? mesh_->GetIndexCount() : 0; }
        nvrhi::Format GetIndexFormat() override { return nvrhi::Format::R16_UINT; }

        PerObjectData& GetPerObjectData() override { return perObjectData_; }
        std::shared_ptr<IObjectShaderResource> GetShaderResource() override { return pointLightShaderRes_; }

        const ObjectMask& GetObjectMask() override { return objectMask_; }
        const DirectX::BoundingBox& GetAABB() override { return aabb_; }

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

        // --- IUpdateGPUData: push light data to the NVRHI constant buffer when dirty ---
        void UpdateGPUData(nvrhi::IDevice* device, nvrhi::ICommandList* cmd) override
        {
            if (!isDirty)
                return;
            isDirty = false;

            EnsureLightBuffer_(device);

            if (pointLightShaderRes_->cb_ && cmd)
                cmd->writeBuffer(pointLightShaderRes_->cb_, &data_, sizeof(PointLightData));
        }

        void SetColor(const DirectX::SimpleMath::Vector3& color)
        {
            data_.color = color;
            isDirty = true;
        }

        Vector3 GetColor() const
        {
            return data_.color;
        }

        void SetRadius(float radius)
        {
            data_.radius = radius;
            std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetTransformComponent().lock()->SetScale({radius, radius, radius});
            isDirty = true;
        }

        float GetRadius()const
        {
            return data_.radius;
        }

        void SetIntensity(float max_intensity)
        {
            data_.max_intensity = max_intensity;
            isDirty = true;
        }

        float GetIntensity()const
        {
            return data_.max_intensity;
        }

        void SetFallOff(float falloff)
        {
            data_.falloff = falloff;
            isDirty = true;
        }

        float GetFallOff()const
        {
            return data_.falloff;
        }

        PointLightData GetData() const { return data_; }

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<std::shared_ptr<TransformComponent>> cashed_event_ = EventHandle<std::shared_ptr<TransformComponent>>::Null();
        PerObjectData perObjectData_;
        std::shared_ptr<PointLightShaderResource> pointLightShaderRes_;
        PointLightData data_;
        ObjectMask objectMask_;
        DirectX::BoundingBox aabb_;
        bool isDirty = true;

        void EnsureLightBuffer_(nvrhi::IDevice* device)
        {
            if (!pointLightShaderRes_->cb_ && device)
            {
                pointLightShaderRes_->cb_ = rhi::createConstantBuffer(device, sizeof(PointLightData), "point light cb");
                pointLightShaderRes_->bindings_.constantBuffer = pointLightShaderRes_->cb_.Get();
            }
        }

        void RefreshCached()
        {
            if (mesh_)
            {
                objectMask_ = mesh_->GetObjectMask().SetLightType(LightType::Point);
                aabb_ = mesh_->GetAABB();
            }
        }

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
