#pragma once
#include <pybind11/conduit/wrap_include_python_h.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directx/d3dx12.h>
#include <DirectXCollision.h>
#include <unordered_map>
#include <variant>
#include <vector>

#include<memory>
#include<json/value.h>
#include<bitset>
#include<filesystem>
#include<optional>

#include<Engine.h>
#include<Component.h>
#include<MeshAsset.h>
#include<Material.h>
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
#include<AssetHandle.h>

namespace GiiGa
{
    class StaticMeshComponent : public Component, public IRenderable, public IUpdateGPUData
    {
        //todo: temp
        friend class ImGuiInspector;
        friend class ImGuiAssetEditor;

    public:
        StaticMeshComponent()=default;
        
        StaticMeshComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
            auto mesh_handle = AssetHandle::FromJson(json["Mesh"]);
            auto material_handle = AssetHandle::FromJson(json["Material"]);

            if (mesh_handle != AssetHandle{})
            {
                mesh_ = Engine::Instance().ResourceManager()->GetAsset<MeshAsset<VertexPNTBT>>(mesh_handle);

                if (material_handle != AssetHandle{})
                {
                    material_ = Engine::Instance().ResourceManager()->GetAsset<Material>(material_handle);
                }
            }
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            result["Type"] = typeid(StaticMeshComponent).name();
            result["Mesh"] = mesh_ ? mesh_->GetId().ToJson() : AssetHandle{}.ToJson();
            result["Material"] = material_ ? material_->GetId().ToJson() : AssetHandle{}.ToJson();
            return result;
        }

        ~StaticMeshComponent()
        {
            if (perObjectData_)
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
                if (!visibilityEntry_ && should_register_)
                    RegisterInVisibility();
            }

            if (!perObjectData_)
            {
                Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);
                perObjectData_ = std::make_shared<PerObjectData>(Engine::Instance().RenderSystem()->GetRenderContext(), transform_.lock(), isStatic_);
            }
        }

        void BeginPlay() override
        {
            
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid,
                                          const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            auto clone = std::make_shared<StaticMeshComponent>();
            this->CloneBase(clone, original_uuid_to_world_uuid, instance_uuid);
            clone->mesh_ = mesh_;
            clone->material_ = material_;
            return clone;
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
        }

        std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent> prefab_comp) const override
        {
            std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>>  result;

            auto prefab_mesh = std::static_pointer_cast<StaticMeshComponent>(prefab_comp);

            if (this->mesh_ != prefab_mesh->mesh_)
                result.push_back({{this->inprefab_uuid_, "Mesh"}, this->mesh_->GetId().ToJson()});

            if (this->material_ != prefab_mesh->material_)
                result.push_back({{this->inprefab_uuid_, "Material"}, this->material_->GetId().ToJson()});

            return result;
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            if (modifications.contains({this->inprefab_uuid_, "Mesh"}))
            {
                auto new_mesh_handle = AssetHandle::FromJson(modifications.at({this->inprefab_uuid_, "Mesh"}));
                this->mesh_ = Engine::Instance().ResourceManager()->GetAsset<MeshAsset<VertexPNTBT>>(new_mesh_handle);
            }

            if (modifications.contains({this->inprefab_uuid_, "Material"}))
            {
                auto new_mat_handle = AssetHandle::FromJson(modifications.at({this->inprefab_uuid_, "Material"}));
                this->material_ = Engine::Instance().ResourceManager()->GetAsset<Material>(new_mat_handle);
            }
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
            //nothing to do
        }

        AssetHandle GetMeshHandle()
        {
            if (mesh_)
                return mesh_->GetId();
            else
                return {};
        }

        void SetDummyMesh(std::shared_ptr<MeshAsset<VertexPNTBT>> asset)
        {
            mesh_ = asset;

            if (!material_)
            {
                auto rm = Engine::Instance().ResourceManager();

                material_ = rm->GetAsset<Material>(DefaultAssetsHandles::DefaultMaterial);
            }

            should_register_ = false;
        }

        void SetMeshHandle(const AssetHandle& new_handle)
        {
            // what a fuck is subresource?
            if (new_handle.id == Uuid::Null())
            {
                mesh_.reset();
                visibilityEntry_.reset();
            }
            else
            {
                mesh_ = Engine::Instance().ResourceManager()->GetAsset<MeshAsset<VertexPNTBT>>(new_handle);
                RegisterInVisibility();

                if (!material_)
                {
                    auto rm = Engine::Instance().ResourceManager();

                    material_ = rm->GetAsset<Material>(DefaultAssetsHandles::DefaultMaterial);
                }
            }
        }

        AssetHandle GetMaterialHandle() const
        {
            if (material_)
                return material_->GetId();
            else
                return {};
        }

        void SetMaterialHandle(const AssetHandle& handle)
        {
            if (handle.id == Uuid::Null())
                material_.reset();
            else
                material_ = Engine::Instance().ResourceManager()->GetAsset<Material>(handle);
        }

        void UpdateGPUData(RenderContext& context) override
        {
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
        bool should_register_ = true;
        //TODO
        //Добавить возможность делать статик меши статическими или динамическими
        bool isStatic_ = false;

        void RegisterInVisibility()
        {
            visibilityEntry_.reset();
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
