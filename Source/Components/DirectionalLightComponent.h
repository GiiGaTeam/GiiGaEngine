#pragma once

#include <DirectXCollision.h>
#include <directxtk12/SimpleMath.h>

#include<memory>
#include<json/value.h>

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
#include<LightComponent.h>
#include<PrefabInstanceModifications.h>
#include<CameraComponent.h>
#include<DXMathUtils.h>
#include<Core/Render/Nvrhi/NvrhiResources.h>

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

    // NVRHI cutover: exposes the directional-light constant buffer via IObjectShaderResource.
    class DirectionLightShaderResource : public IObjectShaderResource
    {
        friend class DirectionalLightComponent;

    public:
        const MaterialBindings& GetMaterialBindings() override { return bindings_; }

    private:
        nvrhi::BufferHandle cb_;
        MaterialBindings bindings_;
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

            data_.cascadeCount = NUM_CASCADE;
            if (auto dev = Engine::Instance().RenderSystem()->GetNvrhiDevice())
                EnsureLightBuffer_(dev);

            shadowTextureSize_ = 2048;
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

                mesh_ = rm->GetAsset<MeshAsset<VertexPNTBT>>(DefaultAssetsHandles::Quad);

                RegisterInVisibility();
            }

            InitShadowData(Engine::Instance().RenderSystem()->GetNvrhiDevice());

            perObjectData_.SetTransform(transform_.lock());
            RefreshCached();
        }

        void BeginPlay() override
        {
        }

        // --- IRenderable: light geometry + data provider ---

        nvrhi::IBuffer* GetVertexBuffer() override { return mesh_ ? mesh_->GetVertexBuffer() : nullptr; }
        nvrhi::IBuffer* GetIndexBuffer() override { return mesh_ ? mesh_->GetIndexBuffer() : nullptr; }
        uint32_t GetIndexCount() override { return mesh_ ? mesh_->GetIndexCount() : 0; }
        nvrhi::Format GetIndexFormat() override { return nvrhi::Format::R16_UINT; }

        PerObjectData& GetPerObjectData() override { return perObjectData_; }
        std::shared_ptr<IObjectShaderResource> GetShaderResource() override { return directionLightShaderRes_; }

        const ObjectMask& GetObjectMask() override { return objectMask_; }
        const DirectX::BoundingBox& GetAABB() override { return aabb_; }

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

        // --- IUpdateGPUData: push light data to the NVRHI constant buffer when dirty ---
        void UpdateGPUData(nvrhi::IDevice* device, nvrhi::ICommandList* cmd) override
        {
            if (!isDirty)
                return;
            isDirty = false;

            EnsureLightBuffer_(device);

            if (transform_.expired()) return;

            Transform trans = transform_.lock()->GetWorldTransform();
            data_.dirWS = trans.GetForward();

            if (directionLightShaderRes_->cb_ && cmd)
                cmd->writeBuffer(directionLightShaderRes_->cb_, &data_, sizeof(DirectionLightData));
        }

        // --- Cascade shadow machinery (NVRHI) ---

        // Recompute per-cascade view/proj from the camera and upload to the cascade structured
        // buffer. Called by ShadowPass once per frame per light.
        void updateCascadeGPUData(nvrhi::IDevice* device, nvrhi::ICommandList* cmd, const Camera& camera)
        {
            UpdateCascadeViewsByCameraData(camera);
            EnsureShadowResources_(device);
            if (cascadeBuffer_ && cmd)
                cmd->writeBuffer(cascadeBuffer_, cascadeData_.data(), sizeof(CascadeData) * NUM_CASCADE);
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

        // Shadow resources (NVRHI). ShadowPass renders into the framebuffer; GLightPass samples
        // the depth array + cascade buffer.
        void InitShadowData(nvrhi::IDevice* device) override
        {
            EnsureShadowResources_(device);
        }

        nvrhi::ITexture* getShadowTexture() const { return shadowDepth_.Get(); }
        nvrhi::IFramebuffer* getShadowFramebuffer() const { return shadowFramebuffer_.Get(); }
        nvrhi::IBuffer* getCascadeBuffer() const { return cascadeBuffer_.Get(); }
        nvrhi::IBuffer* getLightCB() const { return directionLightShaderRes_->cb_.Get(); }
        uint32_t getShadowSize() const { return shadowTextureSize_; }

        void UpdateCascadeViewsByCameraData(const Camera& camera)
        {
            if (transform_.expired()) return;
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
        PerObjectData perObjectData_;
        std::shared_ptr<DirectionLightShaderResource> directionLightShaderRes_;

        DirectionLightData data_;
        std::array<CascadeData, NUM_CASCADE> cascadeData_;

        // NVRHI shadow resources.
        uint32_t shadowTextureSize_ = 2048;
        nvrhi::TextureHandle shadowDepth_;        // D32_FLOAT 2D array, NUM_CASCADE slices
        nvrhi::FramebufferHandle shadowFramebuffer_; // depth-only render target for ShadowPass
        nvrhi::BufferHandle cascadeBuffer_;       // StructuredBuffer<CascadeData> × NUM_CASCADE

        ObjectMask objectMask_;
        DirectX::BoundingBox aabb_;

        float zMult_ = 5; // Multiplication for Ortho projection

        bool isDirty = true;

        void EnsureLightBuffer_(nvrhi::IDevice* device)
        {
            if (!directionLightShaderRes_->cb_ && device)
            {
                directionLightShaderRes_->cb_ = rhi::createConstantBuffer(device, sizeof(DirectionLightData), "directional light cb");
                directionLightShaderRes_->bindings_.constantBuffer = directionLightShaderRes_->cb_.Get();
            }
        }

        void EnsureShadowResources_(nvrhi::IDevice* device)
        {
            if (!device) return;
            if (!shadowDepth_)
            {
                nvrhi::TextureDesc td;
                td.setWidth(shadowTextureSize_).setHeight(shadowTextureSize_)
                  .setFormat(nvrhi::Format::D32)
                  .setArraySize(NUM_CASCADE)
                  .setIsRenderTarget(true)
                  .setInitialState(nvrhi::ResourceStates::DepthWrite)
                  .setDebugName("cascade shadow depth array");
                shadowDepth_ = device->createTexture(td);
            }
            if (!shadowFramebuffer_ && shadowDepth_)
            {
                nvrhi::FramebufferDesc fbd;
                fbd.setDepthAttachment(shadowDepth_);
                shadowFramebuffer_ = device->createFramebuffer(fbd);
            }
            if (!cascadeBuffer_)
            {
                nvrhi::BufferDesc bd;
                bd.setByteSize(sizeof(CascadeData) * NUM_CASCADE)
                  .setStructStride(sizeof(CascadeData))
                  .setInitialState(nvrhi::ResourceStates::ShaderResource)
                  .setKeepInitialState(true)
                  .setDebugName("cascade data structured buffer");
                cascadeBuffer_ = device->createBuffer(bd);
            }
        }

        void RefreshCached()
        {
            if (mesh_)
            {
                objectMask_ = mesh_->GetObjectMask().SetLightType(LightType::Directional);
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
                    isDirty = true;

                    visibilityEntry_->Update(origaabb);
                });
        }
    };
}
