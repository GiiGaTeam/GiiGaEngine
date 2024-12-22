export module StaticMeshComponent;

import <DirectXCollision.h>;
import <memory>;
import <json/value.h>;
import <bitset>;

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


namespace GiiGa
{
    export class StaticMeshComponent : public Component, public IRenderable
    {
    public:
        StaticMeshComponent() = default;

        ~StaticMeshComponent()
        {
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
            if (mesh_)
            {
                if (!material_)
                {
                    
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
            Todo();
        }

        SortData GetSortData() override
        {
            // todo: material shader resource, actually there should not be empty material, we need create default one
            return {.object_mask = mesh_->GetObjectMask(), .shaderResource = nullptr};
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
            }
        }

        Uuid GetMaterialUuid() const
        {
            if (material_)
                return mesh_->GetId().id;
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

    private:
        std::shared_ptr<MeshAsset<VertexPNTBT>> mesh_;
        std::shared_ptr<Material> material_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();

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
