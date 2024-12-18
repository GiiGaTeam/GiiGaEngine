module;

#include <DirectXCollision.h>
#include <memory>
#include <json/value.h>

export module StaticMeshComponent;

import Engine;
import Component;
import Mesh;
import Material;
import IRenderable;
import SceneVisibility;
import GameObject;
import TransformComponent;
import EventSystem;


namespace GiiGa
{
    export class StaticMeshComponent : public Component, public IRenderable, public IVisibilityEntry
    {
    public:
        StaticMeshComponent():
            IVisibilityEntry(std::dynamic_pointer_cast<IRenderable>(this->shared_from_this()))
        {
        }

        void SetOwner(std::shared_ptr<GameObject> go) override
        {
            if (go != nullptr)
                owner_.lock()->GetComponent<TransformComponent>()->OnUpdateTransform.Unregister(cashed_event_);

            Component::SetOwner(go);

            cashed_event_ = go->GetComponent<TransformComponent>()->OnUpdateTransform.Register(
                [this](const UpdateTransformEvent& e)
                {
                    Transform trans = owner_.lock()->GetComponent<TransformComponent>()->GetWorldTransform();
                    DirectX::BoundingBox origaabb = mesh_->GetAABB();

                    origaabb.Transform(origaabb, trans.GetMatrix());

                    IVisibilityEntry::Update(origaabb);
                });
        }

        void Tick(float dt) override;
        void Init() override;
        ::std::shared_ptr<Component> Clone() override;

        // todo
        Json::Value ToJSon() const override
        {
            //mesh_->GetId();
            //material_->GetId();
        }

        // todo
        Json::Value FromJson() const
        {
            //mesh_ = Engine::Instance().ResourceManager().GetAsset<Mesh>(AssetHandle{jsonid, AssetType::Mesh});
            //material_ = Engine::Instance().ResourceManager().GetAsset<Material>(AssetHandle{jsonid, AssetType::Material});
        }

        void Draw(RenderContext& context) override;
        SortData GetSortData() override;

    private:
        std::shared_ptr<Mesh> mesh_;
        std::shared_ptr<Material> material_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
    };
}
