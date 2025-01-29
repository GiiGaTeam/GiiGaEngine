#pragma once
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>

#include "Component.h"
#include "IRenderable.h"
#include "IUpdateGPUData.h"
#include "Skeleton.h"
#include "ozz-animation/PlaybackController.h"

namespace GiiGa
{
    class SkeletalMeshComponent : public Component, public IRenderable, public IUpdateGPUData
    {
        friend class ImGuiInspector;
        friend class ImGuiAssetEditor;
        public:
        SkeletalMeshComponent()=default;

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            result["Type"] = typeid(SkeletalMeshComponent).name();
            result["Mesh"] = mesh_ ? mesh_->GetId().ToJson() : AssetHandle{}.ToJson();
            result["Material"] = material_ ? material_->GetId().ToJson() : AssetHandle{}.ToJson();
            result["Skeleton"] = skeleton_ ? skeleton_->GetId().ToJson() : AssetHandle{}.ToJson();
            return result;
        }

        ~SkeletalMeshComponent()
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
            // Updates current animation time.
            auto animation_ = skeleton_->GetCurrentAnimation()->GetAnimation(); 
            controller_.Update(*animation_, dt);

            // Samples optimized animation at t = animation_time_.
            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = animation_.get();
            sampling_job.context = &skeleton_->context_;
            sampling_job.ratio = controller_.time_ratio();
            sampling_job.output = ozz::make_span(skeleton_->locals_);
            if (!sampling_job.Run()) {
                return ;
            }

            // Converts from local space to model space matrices.
            ozz::animation::LocalToModelJob ltm_job;
            ltm_job.skeleton = skeleton_->GetSkeleton().get();
            ltm_job.input = ozz::make_span(skeleton_->locals_);
            ltm_job.output = ozz::make_span(skeleton_->models_);
            if (!ltm_job.Run()) {
                return ;
            }

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
                perObjectData_ = std::make_shared<PerObjectData>(Engine::Instance().RenderSystem()->GetRenderContext(), transform_.lock(), false);
            }
        }

        void BeginPlay() override
        {
            
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid,
                                          const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            auto clone = std::make_shared<SkeletalMeshComponent>();
            this->CloneBase(clone, original_uuid_to_world_uuid, instance_uuid);
            clone->mesh_ = mesh_;
            clone->material_ = material_;
            clone->skeleton_ = skeleton_;
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

            auto prefab_mesh = std::static_pointer_cast<SkeletalMeshComponent>(prefab_comp);

            if (this->mesh_ != prefab_mesh->mesh_)
                result.push_back({{this->inprefab_uuid_, "Mesh"}, this->mesh_->GetId().ToJson()});

            if (this->material_ != prefab_mesh->material_)
                result.push_back({{this->inprefab_uuid_, "Material"}, this->material_->GetId().ToJson()});

            if (this->skeleton_ != prefab_mesh->skeleton_)
                result.push_back({{this->inprefab_uuid_, "Skeleton"}, this->material_->GetId().ToJson()});
            
            return result;
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            if (modifications.contains({this->inprefab_uuid_, "Mesh"}))
            {
                auto new_mesh_handle = AssetHandle::FromJson(modifications.at({this->inprefab_uuid_, "Mesh"}));
                this->mesh_ = Engine::Instance().ResourceManager()->GetAsset<MeshAsset<VertexBoned>>(new_mesh_handle);
            }

            if (modifications.contains({this->inprefab_uuid_, "Material"}))
            {
                auto new_mat_handle = AssetHandle::FromJson(modifications.at({this->inprefab_uuid_, "Material"}));
                this->material_ = Engine::Instance().ResourceManager()->GetAsset<Material>(new_mat_handle);
            }

            if (modifications.contains({this->inprefab_uuid_, "Skeleton"}))
            {
                auto new_skel_handle = AssetHandle::FromJson(modifications.at({this->inprefab_uuid_, "Skeleton"}));
                this->skeleton_ = Engine::Instance().ResourceManager()->GetAsset<Skeleton>(new_skel_handle);
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

        void RestoreFromLevelJson(const Json::Value&) override
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

        void SetDummyMesh(const std::shared_ptr<MeshAsset<VertexBoned>>& asset)
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
                mesh_ = Engine::Instance().ResourceManager()->GetAsset<MeshAsset<VertexBoned>>(new_handle);
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

        void SetMaterial(std::shared_ptr<Material> mat)
        {
            material_ = mat;
        }
        
        AssetHandle GetSkeletonHandle() const
        {
            if (skeleton_)
                return skeleton_->GetId();
            else
                return {};
        }

        void SetSkeletonHandle(const AssetHandle& handle)
        {
            if (handle.id == Uuid::Null())
                skeleton_.reset();
            else
                skeleton_ = Engine::Instance().ResourceManager()->GetAsset<Skeleton>(handle);
        }

        void SetSkeleton(std::shared_ptr<Skeleton> skeleton)
        {
            skeleton_ = skeleton;
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
        std::shared_ptr<MeshAsset<VertexBoned>> mesh_;
        std::shared_ptr<Skeleton> skeleton_;
        std::shared_ptr<Material> material_;
        std::unique_ptr<VisibilityEntry> visibilityEntry_;
        EventHandle<UpdateTransformEvent> cashed_event_ = EventHandle<UpdateTransformEvent>::Null();
        std::weak_ptr<TransformComponent> transform_;
        std::shared_ptr<PerObjectData> perObjectData_;
        bool should_register_ = true;
        
        ozz::sample::PlaybackController controller_;

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
