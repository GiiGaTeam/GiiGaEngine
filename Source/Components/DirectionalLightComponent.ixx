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

namespace GiiGa
{
    struct alignas(256) DirectionLightData
    {
        DirectX::SimpleMath::Vector3 dirWS;
        float max_intensity = 1;
        alignas(16) DirectX::SimpleMath::Vector3 color = {1, 0, 0};
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
        DirectionalLightComponent():
            directionLightShaderRes_(std::make_shared<DirectionLightShaderResource>())
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();
            auto& context = Engine::Instance().RenderSystem()->GetRenderContext();

            if (isStatic_)
            {
                UINT SizeInBytes = sizeof(DirectionLightData);

                const auto span = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};
                directionLightRes = std::make_unique<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
                directionLightRes->UpdateContentsDeffered(context, span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(directionLightRes->GetResource()->GetGPUVirtualAddress(), SizeInBytes);
                directionLightShaderRes_->directionLightCBV_ = directionLightRes->CreateConstantBufferView(desc);
            }
        }

        ~DirectionalLightComponent()
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
        }

        ::std::shared_ptr<IComponent> Clone() override
        {
            Todo();
            return {};
        }

        void Draw(RenderContext& context) override
        {
            mesh_->Draw(context.GetGraphicsCommandList());
        }

        SortData GetSortData() override
        {
            return {.object_mask = mesh_->GetObjectMask().SetFillMode(FillMode::Solid).SetLightType(LightType::Direction), .shaderResource = directionLightShaderRes_};
        }

        void Restore(const Json::Value&) override
        {
            Todo();
        }

        Json::Value DerivedToJson() override
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
                directionLightRes->UpdateContentsImmediate(context, span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
                directionLightShaderRes_->directionLightCBV_ = context.AllocateDynamicConstantView(span, desc);
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

        void SetIntensity(float max_intensity) override
        {
            data_.max_intensity = max_intensity;
            isDirty = true;
        }

        DirectionLightData GetData() const {return data_;}

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
        std::shared_ptr<PerObjectData> perObjectData_;
        std::unique_ptr<GPULocalResource> directionLightRes;
        std::shared_ptr<DirectionLightShaderResource> directionLightShaderRes_;
        DirectionLightData data_;

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
