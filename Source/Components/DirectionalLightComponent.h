#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <DirectXCollision.h>
#include <directxtk12/SimpleMath.h>
#include <nvrhi/nvrhi.h>

#include<memory>
#include<json/value.h>


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
#include<NvrhiUtils.h>
#include<LightComponent.h>
#include<PrefabInstanceModifications.h>
#include<CameraComponent.h>
#include<DXMathUtils.h>

using namespace DirectX::SimpleMath;

namespace GiiGa
{
    struct alignas(256) DirectionLightData
    {
        DirectX::SimpleMath::Vector3 dirWS;
        float max_intensity = 1;
        DirectX::SimpleMath::Vector3 color = {1, 0, 0};
        float cascadeCount;

        DirectionLightData() = default;

        DirectionLightData(const Json::Value& json)
        {
            max_intensity = json["max_intensity"].asFloat();
            color = Vector3FromJson(json["color"]);
            cascadeCount = json["cascadeCount"].asFloat();
        }

        Json::Value toJson()
        {
            Json::Value json;

            json["max_intensity"] = max_intensity;
            json["color"] = Vector3ToJson(color);
            json["cascadeCount"] = cascadeCount;

            return json;
        }
    };

    class DirectionLightShaderResource : public IObjectShaderResource
    {
        friend class DirectionalLightComponent;

    public:
        std::vector<nvrhi::BindingSetItem> GetBindingItems() override
        {
            // canonical light constant buffer slot: b3
            return {nvrhi::BindingSetItem::ConstantBuffer(3, directionLightCB_)};
        }

    private:
        nvrhi::BufferHandle directionLightCB_;
    };

    class DirectionalLightComponent : public LightComponent
    {
    public:
        static inline const uint8_t NUM_CASCADE = 4; // Если меняешь, то не забудь в CascadeShadowGeomShader.hlsl instance поменять

        struct CascadeData
        {
            Matrix View;
            Matrix Proj;
            float Distances;
        };

        DirectionalLightComponent():
            directionLightShaderRes_(std::make_shared<DirectionLightShaderResource>())
        {
            ConstructFunction();
        }

        DirectionalLightComponent(const Json::Value& json, bool roll_id = false):
            LightComponent(json, roll_id),
            directionLightShaderRes_(std::make_shared<DirectionLightShaderResource>()),
            data_(json["DirectionLightData"])
        {
            ConstructFunction();
        }

        void ConstructFunction()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();
            auto& context = Engine::Instance().RenderSystem()->GetRenderContext();

            data_.cascadeCount = NUM_CASCADE;

            directionLightShaderRes_->directionLightCB_ = CreateConstantBuffer(device.NvDevice(), sizeof(DirectionLightData), "DirectionLightData");
            const auto span = std::span{reinterpret_cast<const uint8_t*>(&data_), sizeof(DirectionLightData)};
            context.WriteBufferDeffered(directionLightShaderRes_->directionLightCB_, span);

            cascadeDataBuffer_ = CreateStructuredBuffer(device.NvDevice(), sizeof(CascadeData) * NUM_CASCADE, sizeof(CascadeData), "CascadeData");

            TEXTURE_SIZE = Vector2(2048);
            DS_FORMAT = nvrhi::Format::D32;
            shadow_views.reserve(NUM_CASCADE);
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value json;

            json["Type"] = typeid(DirectionalLightComponent).name();
            json["DirectionLightData"] = data_.toJson();

            return json;
        }

        ~DirectionalLightComponent() override
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

                mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(DefaultAssetsHandles::Quad);

                RegisterInVisibility();
            }

            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();
            InitShadowData(device);
        }

        void BeginPlay() override
        {
        }

        void Draw(RenderContext& context) override
        {
            mesh_->Draw(context);
        }

        SortData GetSortData() override
        {
            return {
                .object_mask = mesh_->GetObjectMask()
                                    .SetLightType(LightType::Directional),
                .shaderResource = directionLightShaderRes_
            };
        }

        void RestoreFromLevelJson(const Json::Value&) override
        {
            // nothing to restore
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            Todo();
            return {};
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        std::vector<std::pair<::GiiGa::PropertyModificationKey, ::GiiGa::PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PrefabPropertyModifications& modifications) override
        {
            Todo();
        }

        void UpdateGPUData(RenderContext& context) override
        {
            if (!isDirty)
                return;

            if (!perObjectData_)
                perObjectData_ = std::make_shared<PerObjectData>(context, transform_.lock(), isStatic_);
            perObjectData_->UpdateGPUData(context);

            if (transform_.expired()) return;

            Transform trans = transform_.lock()->GetWorldTransform();
            data_.dirWS = trans.GetForward();

            const auto span = std::span{reinterpret_cast<const uint8_t*>(&data_), sizeof(DirectionLightData)};
            context.WriteBufferImmediate(directionLightShaderRes_->directionLightCB_, span);
        }

        void UpdateCascadeGPUData(RenderContext& context, Camera& camera)
        {
            UpdateCascadeViewsByCameraData(camera);
            const auto span_shadow = std::span{reinterpret_cast<const uint8_t*>(cascadeData_.data()), sizeof(CascadeData) * NUM_CASCADE};
            context.WriteBufferImmediate(cascadeDataBuffer_, span_shadow);
        }

        PerObjectData& GetPerObjectData() override
        {
            return *perObjectData_;
        }

        void SetColor(const DirectX::SimpleMath::Vector3& color)
        {
            data_.color = color;
            isDirty = true;
        }

        void SetIntensity(float max_intensity)
        {
            data_.max_intensity = max_intensity;
            isDirty = true;
        }

        DirectionLightData GetData() const { return data_; }


        void InitShadowData(RenderDevice& device) override
        {
            nvrhi::TextureDesc desc;
            desc.width = static_cast<uint32_t>(TEXTURE_SIZE.x);
            desc.height = static_cast<uint32_t>(TEXTURE_SIZE.y);
            desc.arraySize = NUM_CASCADE;
            desc.dimension = nvrhi::TextureDimension::Texture2DArray;
            desc.format = DS_FORMAT;
            desc.isRenderTarget = true;
            desc.initialState = nvrhi::ResourceStates::DepthWrite;
            desc.keepInitialState = true;
            desc.clearValue = nvrhi::Color(1.0f, 0.0f, 0.0f, 0.0f);
            desc.useClearValue = true;
            desc.debugName = "DirectionalLightShadowMap";

            shadow_map_ = device.NvDevice()->createTexture(desc);

            nvrhi::FramebufferDesc fb_desc;
            // the attachment must span all cascade slices; the geometry shader routes
            // triangles to them via SV_RenderTargetArrayIndex
            fb_desc.setDepthAttachment(shadow_map_, nvrhi::TextureSubresourceSet(0, 1, 0, NUM_CASCADE));
            shadow_framebuffer_ = device.NvDevice()->createFramebuffer(fb_desc);
        }

        void UpdateCascadeViewsByCameraData(const Camera& camera)
        {
            const auto transform = transform_.lock()->GetTransform();
            const auto forward = transform.GetForward();
            const auto up = transform.GetUp();

            float percentDist = 1.0f / static_cast<float>(NUM_CASCADE);
            for (uint32_t i = 0; i < NUM_CASCADE; ++i)
            {
                Matrix subProj;
                camera.GetSubProjAndDistanceToFar(percentDist * i, percentDist * static_cast<float>(i + 1), subProj,
                                                  cascadeData_[i].Distances);
                const auto corners = ExtractFrustumWorldCorners(camera.GetView() * subProj);
                const auto center = GetFrustumCenter(corners);
                const auto tar = center + forward;
                const auto view = Matrix::CreateLookAt(center, tar, Vector3::Up);
                const auto proj = GetOrthographicProjByCorners(corners, view);
                cascadeData_[i].View = view.Transpose();
                cascadeData_[i].Proj = proj.Transpose();
            }
        }

        std::vector<Matrix> GetViews() override
        {
            shadow_views.clear();
            for (const auto& cascade : cascadeData_)
            {
                shadow_views.push_back(cascade.View.Transpose() * cascade.Proj.Transpose());
            }
            return shadow_views;
        }

        nvrhi::IBuffer* GetCascadeDataBuffer() const
        {
            return cascadeDataBuffer_;
        }

        nvrhi::IBuffer* GetLightDataCB() const
        {
            return directionLightShaderRes_->directionLightCB_;
        }

    protected:
        Matrix GetOrthographicProjByCorners(const std::array<Vector3, 8>& corners, const Matrix& lightView) const
        {
            Vector4 FrustumMin;
            Vector4 FrustumMax;
            FrustumMin.x = FLT_MAX;
            FrustumMax.x = std::numeric_limits<float>::lowest();
            FrustumMin.y = FLT_MAX;
            FrustumMax.y = std::numeric_limits<float>::lowest();
            FrustumMin.z = FLT_MAX;
            FrustumMax.z = std::numeric_limits<float>::lowest();

            for (const auto& corner : corners)
            {
                const auto corner4 = Vector4(corner.x, corner.y, corner.z, 1.0f);
                Vector4 vTempTranslateCornerPoints = Vector4::Transform(corner4, lightView);

                Vector4::Min(FrustumMin, vTempTranslateCornerPoints, FrustumMin);
                Vector4::Max(FrustumMax, vTempTranslateCornerPoints, FrustumMax);
            }

            FrustumMin.z = FrustumMin.z < 0 ? FrustumMin.z * zMult_ : FrustumMin.z / zMult_;
            FrustumMax.z = FrustumMax.z < 0 ? FrustumMax.z / zMult_ : FrustumMax.z * zMult_;

            const auto resOrthoMat = Matrix::CreateOrthographicOffCenter(FrustumMin.x, FrustumMax.x,
                                                                         FrustumMin.y, FrustumMax.y, FrustumMin.z, FrustumMax.z);

            return resOrthoMat;
        }

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<std::shared_ptr<TransformComponent>> cashed_event_ = EventHandle<std::shared_ptr<TransformComponent>>::Null();
        std::shared_ptr<PerObjectData> perObjectData_;
        std::shared_ptr<DirectionLightShaderResource> directionLightShaderRes_;
        nvrhi::BufferHandle cascadeDataBuffer_;

        DirectionLightData data_;
        std::array<CascadeData, NUM_CASCADE> cascadeData_;

        float zMult_ = 5; // Multiplication for Ortho projection

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
                    isDirty = true;

                    visibilityEntry_->Update(origaabb);
                });
        }
    };
}
