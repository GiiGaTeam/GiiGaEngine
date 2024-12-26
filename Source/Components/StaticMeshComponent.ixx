module;

#include <directx/d3dx12.h>
#include <DirectXCollision.h>

export module StaticMeshComponent;

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

namespace GiiGa
{
    export class StaticMeshComponent : public Component, public IRenderable, public IUpdateGPUData
    {
    public:
        StaticMeshComponent()
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);
        }

        ~StaticMeshComponent()
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
            if (mesh_)
            {
                if (!material_)
                {
                    auto rm = Engine::Instance().ResourceManager();

                    material_ = rm->GetAsset<Material>(DefaultAssetsHandles::DefaultMaterial);
                }
                if (!visibilityEntry_)
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
            return {.object_mask = mesh_->GetObjectMask() | material_->GetMaterialMask(), .shaderResource = material_->GetShaderResource()};
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

        Uuid GetMeshUuid()
        {
            if (mesh_)
                return mesh_->GetId().id;
            else
                return Uuid::Null();
        }

        void SetMeshUuid(const Uuid& newUuid)
        {
            // what a fuck is subresource?
            if (newUuid == Uuid::Null())
            {
                mesh_.reset();
                visibilityEntry_.reset();
            }
            else
            {
                mesh_ = Engine::Instance().ResourceManager()->GetAsset<MeshAsset<VertexPNTBT>>({newUuid, 0});
                RegisterInVisibility();

                if (!material_)
                {
                    auto rm = Engine::Instance().ResourceManager();

                    material_ = rm->GetAsset<Material>(DefaultAssetsHandles::DefaultMaterial);
                }
            }
        }

        Uuid GetMaterialUuid() const
        {
            if (material_)
                return material_->GetId().id;
            else
                return Uuid::Null();
        }

        void SetMaterialUuid(const Uuid& newUuid)
        {
            // what a fuck is subresource?
            if (newUuid == Uuid::Null())
                material_.reset();
            else
                material_ = Engine::Instance().ResourceManager()->GetAsset<Material>({newUuid, 0});
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
        std::shared_ptr<Material> material_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
        std::weak_ptr<TransformComponent> transform_;
        std::shared_ptr<PerObjectData> perObjectData_;
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
