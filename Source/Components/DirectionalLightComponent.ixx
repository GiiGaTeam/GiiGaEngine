module;

#define NOMINMAX
#include <directx/d3dx12.h>
#include <DirectXCollision.h>
#include <directxtk12/SimpleMath.h>

export module DirectionalLightComponent;

import <memory>;
import <json/value.h>;
import <bitset>;
import <filesystem>;
import <algorithm>;
using namespace DirectX::SimpleMath;

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
import LightComponent;
import PrefabInstance;
import GPULocalResource;
import CameraComponent;
import MathUtils;

namespace GiiGa
{
    struct alignas(256) DirectionLightData
    {
        DirectX::SimpleMath::Vector3 dirWS;
        float max_intensity = 1;
        DirectX::SimpleMath::Vector3 color = {1, 0, 0};
        float cascadeCount;
    };

    export class DirectionLightShaderResource : public IObjectShaderResource
    {
        friend class DirectionalLightComponent;

    public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result(1);
            result[0] = directionLightCBV_->getDescriptor().getGPUHandle();
            return result;
        }

    private:
        std::shared_ptr<BufferView<Constant>> directionLightCBV_;
    };

    export class DirectionalLightComponent : public LightComponent
    {
    public:
        static inline const uint8_t NUM_CASCADE = 4;

        struct alignas(256) CascadeData
        {
            Matrix ViewProj;
            float Distances;
        };

        DirectionalLightComponent():
            directionLightShaderRes_(std::make_shared<DirectionLightShaderResource>())
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();
            auto& context = Engine::Instance().RenderSystem()->GetRenderContext();

            data_.cascadeCount = NUM_CASCADE;
            if (isStatic_)
            {
                UINT SizeInBytes = sizeof(DirectionLightData);

                const auto span = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};
                directionLightRes_ = std::make_unique<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
                directionLightRes_->UpdateContentsDeffered(context, span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(directionLightRes_->GetResource()->GetGPUVirtualAddress(), SizeInBytes);
                directionLightShaderRes_->directionLightCBV_ = directionLightRes_->CreateConstantBufferView(desc);
            }

            TEXTURE_SIZE = Vector2(800);
            DS_FORMAT_RES = DXGI_FORMAT_R32_TYPELESS;
            DS_FORMAT_DSV = DXGI_FORMAT_D32_FLOAT;
            DS_FORMAT_SRV = DXGI_FORMAT_R32_FLOAT;
            DS_DIMENSION_RES = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            DS_DIMENSION_DSV = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            DS_DIMENSION_SRV = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            DS_CLEAR_VALUE = {.Format = DS_FORMAT_DSV, .DepthStencil = {.Depth = 1, .Stencil = 1}};
            shadow_views.reserve(NUM_CASCADE);
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

        void Draw(RenderContext& context) override
        {
            mesh_->Draw(context.GetGraphicsCommandList());
        }

        SortData GetSortData() override
        {
            return {.object_mask = mesh_->GetObjectMask().SetFillMode(FillMode::Solid).SetLightType(LightType::Directional), .shaderResource = directionLightShaderRes_};
        }

        void Restore(const Json::Value&) override
        {
            Todo();
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

        std::vector<std::pair<::GiiGa::PropertyModificationKey, ::GiiGa::PropertyValue>> GetModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PropertyModifications& modifications) override
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

            UINT SizeInBytes = sizeof(DirectionLightData);

            const auto span = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};

            if (isStatic_)
            {
                directionLightRes_->UpdateContentsImmediate(context, span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
                directionLightShaderRes_->directionLightCBV_ = context.AllocateDynamicConstantView(span, desc);
            }
        }

        void UpdateCascadeGPUData(RenderContext& context, Camera& camera)
        {
            UpdateCascadeViewsByCameraData(camera);
            const auto SizeInBytes = sizeof(CascadeData) * NUM_CASCADE;
            const auto span_shadow = std::span{reinterpret_cast<uint8_t*>(&cascadeData_), SizeInBytes};
            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = NUM_CASCADE; // Количество элементов в буфере
            desc.Buffer.StructureByteStride = sizeof(CascadeData);
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            shadowShaderRes_ = context.AllocateDynamicShaderResourceView(span_shadow, desc);
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

        void SetIntensity(float max_intensity) override
        {
            data_.max_intensity = max_intensity;
            isDirty = true;
        }

        DirectionLightData GetData() const { return data_; }


        void InitShadowData(RenderDevice& device) override
        {
            {
                D3D12_RESOURCE_DESC rtvDesc = {};
                rtvDesc.Format = DS_FORMAT_RES; // Use a format without stencil
                rtvDesc.Dimension = DS_DIMENSION_RES;
                rtvDesc.Alignment = 0;
                rtvDesc.Width = static_cast<UINT>(TEXTURE_SIZE.x);
                rtvDesc.Height = static_cast<UINT>(TEXTURE_SIZE.y);
                rtvDesc.DepthOrArraySize = NUM_CASCADE;
                rtvDesc.MipLevels = 1;
                rtvDesc.SampleDesc.Count = 1; // No multisampling
                rtvDesc.SampleDesc.Quality = 0;
                rtvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                D3D12_CLEAR_VALUE clear_value = DS_CLEAR_VALUE;
                shadow_resource_ = std::make_unique<GPULocalResource>(device, rtvDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear_value);
            }

            {
                D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
                dsv_desc.Format = DS_FORMAT_DSV; // Use a format without stencil
                dsv_desc.ViewDimension = DS_DIMENSION_DSV;
                dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
                dsv_desc.Texture2DArray.MipSlice = 0; // Use the first mip level
                dsv_desc.Texture2DArray.FirstArraySlice = 0;
                dsv_desc.Texture2DArray.ArraySize = NUM_CASCADE;

                shadow_DSV_ = shadow_resource_->EmplaceDepthStencilView(dsv_desc);
            }

            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DS_FORMAT_SRV; // Specify the format of the SRV
                srvDesc.ViewDimension = DS_DIMENSION_SRV;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2DArray.MostDetailedMip = 0;        // Use the most detailed mip level
                srvDesc.Texture2DArray.MipLevels = 1;              // Use one mip level
                srvDesc.Texture2DArray.PlaneSlice = 0;             // Only relevant for certain texture formats, usually set to 0
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f; // Resource minimum level-of-detail clamp
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = NUM_CASCADE;

                // Create the Shader Resource View
                shadow_SRV_ = shadow_resource_->EmplaceShaderResourceBufferView(srvDesc);
            }
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
                //cascData.Distances[i] /= 50.0f;
                const auto corners = ExtractFrustumWorldCorners(camera.GetViewProj());
                const auto center = GetFrustumCenter(corners);
                const auto tar = center + forward;
                const auto view = Matrix::CreateLookAt(center, tar, up);
                const auto proj = GetOrthographicProjByCorners(corners, view);
                cascadeData_[i].ViewProj = (view * proj).Transpose();
            }
        }

        std::vector<Matrix> GetViews() override
        {
            shadow_views.clear();
            for (const auto& cascade : cascadeData_)
            {
                shadow_views.push_back(cascade.ViewProj);
            }
            return shadow_views;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetCascadeDataSRV() const
        {
            return shadowShaderRes_->getDescriptor().getGPUHandle();
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetLightDataSRV() const
        {
            return directionLightShaderRes_->directionLightCBV_->getDescriptor().getGPUHandle();
        }

    protected:
        Matrix GetOrthographicProjByCorners(const std::array<Vector3, 8>& corners, const Matrix& lightView) const
        {
            if (corners.size() != 8) return Matrix::Identity;

            Vector3 FrustumMin;
            Vector3 FrustumMax;
            FrustumMin.x = FLT_MAX;
            FrustumMax.x = std::numeric_limits<float>::lowest();
            FrustumMin.y = FLT_MAX;
            FrustumMax.y = std::numeric_limits<float>::lowest();
            FrustumMin.z = FLT_MAX;
            FrustumMax.z = std::numeric_limits<float>::lowest();

            for (const auto& corner : corners)
            {
                Vector3 vTempTranslateCornerPoints = Vector3::Transform(corner, lightView);

                Vector3::Min(FrustumMin, vTempTranslateCornerPoints, FrustumMin);
                Vector3::Max(FrustumMax, vTempTranslateCornerPoints, FrustumMax);
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
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
        std::shared_ptr<PerObjectData> perObjectData_;
        std::unique_ptr<GPULocalResource> directionLightRes_;
        std::shared_ptr<DirectionLightShaderResource> directionLightShaderRes_;
        std::shared_ptr<BufferView<ShaderResource>> shadowShaderRes_;

        DirectionLightData data_;
        std::array<CascadeData, NUM_CASCADE> cascadeData_;

        const float zMult_ = 10.0f; // Multiplication for Ortho projection

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

                    data_.dirWS = trans.GetForward();
                    isDirty = true;

                    visibilityEntry_->Update(origaabb);
                });
        }
    };
}
