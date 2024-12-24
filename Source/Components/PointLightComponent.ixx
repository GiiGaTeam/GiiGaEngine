module;

#include <directx/d3dx12.h>
#include <DirectXCollision.h>

export module PointLightComponent;

import <memory>;
import <json/value.h>;
import <bitset>;
import <filesystem>;

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

namespace GiiGa
{
    export class PointLightShaderResource : public IObjectShaderResource
    {

    public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result;
            return result;
        }

    private:
        std::shared_ptr<BufferView<Constant>> PointLightCBV_;
    };
    
    export class PointLightComponent : public Component, public IRenderable, public IUpdateGPUData
    {
    public:
        PointLightComponent()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);
            pointLightShaderRes_ = std::make_shared<PointLightShaderResource>();
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
            return {.object_mask = mesh_->GetObjectMask().SetFillMode(FillMode::Wire), .shaderResource = pointLightShaderRes_};
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
            if (!perObjectData_)
                perObjectData_ = std::make_shared<PerObjectData>(context, transform_.lock(), isStatic_);
            perObjectData_->UpdateGPUData(context);
        }

        PerObjectData& GetPerObjectData() override
        {
            return *perObjectData_;
        }

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
        std::weak_ptr<TransformComponent> transform_;
        std::shared_ptr<PerObjectData> perObjectData_;
        std::shared_ptr<PointLightShaderResource> pointLightShaderRes_;
        //TODO
        //Добавить возможность делать статик меши статическими или динамическими
        bool isStatic_ = false;

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

                    visibilityEntry_->Update(origaabb);
                });
        }
    };
}
