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
import TransformComponent;
import EventSystem;
import GameObject;
import Misc;


namespace GiiGa
{
    export class StaticMeshComponent : public Component, public IRenderable, public IVisibilityEntry
    {
    public:
        StaticMeshComponent():
            IVisibilityEntry(std::dynamic_pointer_cast<IRenderable>(this->shared_from_this()))
        {
        }

        void SetOwner(std::shared_ptr<IGameObject> go) override
        {
            auto go_go = std::dynamic_pointer_cast<GameObject>(go);
            auto owner_go = std::dynamic_pointer_cast<GameObject>(owner_.lock());
            
            if (go_go != nullptr)
                std::dynamic_pointer_cast<GameObject>(owner_.lock())->GetComponent<TransformComponent>()->OnUpdateTransform.Unregister(cashed_event_);

            Component::SetOwner(go_go);

            cashed_event_ = go_go->GetComponent<TransformComponent>()->OnUpdateTransform.Register(
                [this](const UpdateTransformEvent& e)
                {
                    auto owner_go = std::dynamic_pointer_cast<GameObject>(owner_.lock());
                    Transform trans = owner_go->GetComponent<TransformComponent>()->GetWorldTransform();
                    DirectX::BoundingBox origaabb = mesh_->GetAABB();

                    origaabb.Transform(origaabb, trans.GetMatrix());

                    IVisibilityEntry::Update(origaabb);
                });
        }

        void Tick(float dt) override;
        void Init() override;
        ::std::shared_ptr<IComponent> Clone() override
        {
            Todo();
        }

        // todo
        Json::Value ToJson() override
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

        void Draw() override;
        SortData GetSortData() override;

    private:
        std::shared_ptr<Mesh> mesh_;
        std::shared_ptr<Material> material_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
    };
}
