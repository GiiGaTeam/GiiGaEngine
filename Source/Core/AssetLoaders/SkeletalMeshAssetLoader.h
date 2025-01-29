#pragma once
#include<DirectXCollision.h>

#include<stdexcept>
#include<filesystem>
#include<memory>
#include<vector>
#include<directxtk12/SimpleMath.h>

#include<Uuid.h>
#include<AssetLoader.h>
#include<AssetType.h>
#include<ConcreteAsset/MeshAsset.h>
#include<Engine.h>
#include<VertexTypes.h>
#include<AssetHandle.h>
#include <easylogging++.h>

#include<ConcreteAsset/PrefabAsset.h>
#include<TransformComponent.h>
#include<StaticMeshComponent.h>
#include<GameObject.h>


#include<EditorAssetDatabase.h>
#include <ozz/base/log.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include "Animation.h"
#include "AnimationAssetLoader.h"
#include "SkeletalMeshComponent.h"
#include "Skeleton.h"
#include "SkeletonAssetLoader.h"
#include "ozz-animation/mesh.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace GiiGa
{
    class SkeletalMeshAssetLoader : public AssetLoader
    {
    public:
        virtual ~SkeletalMeshAssetLoader() = default;

        SkeletalMeshAssetLoader()
        {
            id_ = Uuid::FromString("bebbf9e4-8e31-4980-bdc2-12058f2bf40c").value();
            pattern_ = R"(.+)\.fbx_animation_data\mesh.ozz)";
            type_ = AssetType::SkeletalMesh;
        }

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
        {
            if (!std::filesystem::exists(absolute_path))
                throw std::runtime_error("File does not exist: " + absolute_path.string());

            auto materials_map = GetMaterialsFromJSON(absolute_path.parent_path() / "materials_info.json");
            auto skeleton = PreprocessSkeleton(absolute_path.parent_path() / "skeleton.ozz");
            
            ozz::vector<ozz::sample::Mesh> meshes;
            LoadSkeletalMesh(reinterpret_cast<const char*>(relative_path.c_str()), &meshes);

            std::unordered_map<AssetHandle, AssetMeta> handles;
            
            std::string parent_path = absolute_path.parent_path().string();
            size_t pos = parent_path.find("_animation_data");
            std::string mesh_name = parent_path.substr(0, pos);

            auto uuid = Uuid::New();
            auto go = CreateGameObjectWithSkeletalMesh(
                mesh_name,
                &meshes,
                relative_path,
                handles,
                materials_map,
                skeleton
            );

            if (auto db = std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().AssetDatabase()))
            {
                std::filesystem::path path = absolute_path;
                path = path.parent_path() / (path.stem().string() + ".prefab");

                auto prefab = std::make_shared<PrefabAsset>(AssetHandle{Uuid::New(), 0}, go);
                db->CreateAsset(prefab, path);
            }

            return handles;
        }

        std::shared_ptr<AssetBase> Load(AssetHandle handle, const std::filesystem::path& path) override
        {
            if (!std::filesystem::exists(path))
                throw std::runtime_error("File does not exist: " + path.string());

            auto rs = Engine::Instance().RenderSystem();
            auto device = rs->GetRenderDevice().GetDxDevice();
            
            ozz::vector<ozz::sample::Mesh> meshes;
            LoadSkeletalMesh(reinterpret_cast<const char*>(path.c_str()), &meshes);
            auto mesh = meshes[handle.subresource];

            std::vector<VertexBoned> vertices = ProcessVertices(&mesh);
            std::vector<Index16> indices;
            std::ranges::transform(mesh.triangle_indices, indices.begin(),
                                   [](uint16_t idx) { return Index16{idx}; });

            DirectX::BoundingBox aabb = CalculateBoundingBox(vertices);
            
            auto mesh_asset = std::make_shared<MeshAsset<VertexBoned>>(handle, rs->GetRenderContext(), rs->GetRenderDevice(), vertices, indices, aabb);
            //mesh_asset->SetSkeltalMesh(std::make_shared<ozz::sample::Mesh>(mesh));
            return mesh_asset;
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving Mesh is not supported.");
        }

        const char* GetName() override
        {
            return "Mesh Loader";
        }

    private:

        void LoadSkeletalMesh(const char* _filename, ozz::vector<ozz::sample::Mesh>* _meshes)
        {
            ozz::log::Out() << "Loading meshes archive: " << _filename << "."
                  << std::endl;
            ozz::io::File file(_filename, "rb");
            if (!file.opened()) {
                ozz::log::Err() << "Failed to open mesh file " << _filename << "."
                                << std::endl;
            }
            ozz::io::IArchive archive(&file);

            {
                while (archive.TestTag<ozz::sample::Mesh>()) {
                    _meshes->resize(_meshes->size() + 1);
                    archive >> _meshes->back();
                }
            }
        }

        std::shared_ptr<GameObject> CreateGameObjectWithSkeletalMesh
        (
            std::string name,
            ozz::vector<ozz::sample::Mesh>* _meshes,
            const std::filesystem::path& relative_path,
            std::unordered_map<AssetHandle, AssetMeta>& handles,
            std::unordered_map<int, std::shared_ptr<Material>> material_map,
            std::shared_ptr<Skeleton> skeleton
            )
        {
            auto gameObject = GameObject::CreateEmptyGameObject(SpawnParameters{
                name,
                std::weak_ptr<IGameObject>(),
                std::weak_ptr<ILevelRootGameObjects>(),
            });
            
            gameObject->GetTransformComponent().lock()->SetTransform(Transform());

            for (size_t i = 0; i < _meshes->size(); ++i)
            {
                auto handle = AssetHandle{Uuid::New(), 0};
                auto mesh_asset = std::make_shared<MeshAsset<VertexBoned>>(handle);

                auto skeletalMeshComponent = gameObject->CreateComponent<SkeletalMeshComponent>();
                skeletalMeshComponent->SetDummyMesh(mesh_asset);

                handles.emplace(handle, AssetMeta{
                                            type_,
                                            relative_path,
                                            id_,
                                            name,
                                        });

                if (material_map.contains(i))
                    skeletalMeshComponent->SetMaterial(material_map.at(i));

                if (skeleton)
                    skeletalMeshComponent->SetSkeleton(skeleton);
                
            }
            return gameObject;
        }

        std::shared_ptr<Skeleton> PreprocessSkeleton(const std::filesystem::path& skeleton_path)
        {
            std::shared_ptr<Skeleton> skeleton_asset;
            auto skeleton_handles = Engine::Instance().AssetDatabase()->GetHandlesByPath(skeleton_path.c_str());
            if (!skeleton_handles.empty())
                skeleton_asset = Engine::Instance().ResourceManager()->GetAsset<Skeleton>(std::get<AssetHandle>(skeleton_handles[0]));
            else
            {
                skeleton_asset = std::dynamic_pointer_cast<Skeleton>(SkeletonAssetLoader().Load(AssetHandle{Uuid::New(), 0}, skeleton_path));
                std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().AssetDatabase())->CreateAsset(skeleton_asset, skeleton_path, false);
            }
            
            // Попробуем здесь поработать ещё и с анимациями?...
            // Вряд ли нам понадобится ещё ссылка на них

            for (auto const& dir_entry : std::filesystem::directory_iterator(skeleton_path.parent_path()))
            {
                if (dir_entry.path().filename() == "skeleton.ozz")
                    continue;
                
                auto animation_handles = Engine::Instance().AssetDatabase()->GetHandlesByPath(dir_entry.path().c_str());
                if (animation_handles.empty())
                {
                    auto animation_asset = std::dynamic_pointer_cast<Animation>(AnimationAssetLoader().Load(AssetHandle{Uuid::New(), 0}, dir_entry.path().filename()));
                    std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().AssetDatabase())->CreateAsset(animation_asset, dir_entry.path().filename(), false);
                    skeleton_asset->AddAnimation(animation_asset);
                }
            }

            return skeleton_asset;
        }

        std::unordered_map<int, std::shared_ptr<Material>> GetMaterialsFromJSON(const std::filesystem::path& mat_path)
        {
            std::unordered_map<int, std::shared_ptr<Material>> materials_map;

            if (!std::filesystem::exists(mat_path))
            {
                auto msg = "Material file not found in: " + mat_path.string();
                el::Loggers::getLogger(LogResourceManager)->warn(msg);
                throw std::runtime_error(msg);
            }

            std::ifstream mats_file(mat_path);
            if (!mats_file.is_open())
            {
                auto msg = "Failed to open Material file: " + mat_path.string();
                el::Loggers::getLogger(LogResourceManager)->warn(msg);
                throw std::runtime_error(msg);
            }

            Json::CharReaderBuilder reader_builder;
            Json::Value mats_json;
            std::string errs;

            if (!Json::parseFromStream(reader_builder, mats_file, &mats_json, &errs))
                throw std::runtime_error("Failed to parse Material file: " + errs);

            for (auto& mat : mats_json)
            {
                auto asset_handle = AssetHandle::FromJson(mat["material_mesh_info"]);
                auto material = Engine::Instance().ResourceManager()->GetAsset<Material>(asset_handle);
                materials_map.emplace(asset_handle.subresource, material);
            }
            return materials_map;
            
        }

        std::vector<VertexBoned> ProcessVertices(ozz::sample::Mesh* mesh)
        {
            std::vector<VertexBoned> vertices(mesh->triangle_index_count());
            for (size_t i = 0; i < mesh->parts.size(); ++i)
            {
                auto part_vertex_count = mesh->parts[i].vertex_count();
                for (size_t j = 0; j < part_vertex_count; ++j)
                {
                    Vector3 position = Vector3{mesh->parts[i].positions[j*3], mesh->parts[i].positions[j*3+1], mesh->parts[i].positions[j*3+2]};
                    Vector3 normal = Vector3{mesh->parts[i].normals[j*3], mesh->parts[i].normals[j*3+1], mesh->parts[i].normals[j*3+2]};
                    Vector3 tangent = Vector3{mesh->parts[i].tangents[j*3], mesh->parts[i].tangents[j*3+1], mesh->parts[i].tangents[j*3+2]};
                    Vector2 texCoord = Vector2{mesh->parts[i].uvs[j*2], mesh->parts[i].uvs[j*2+1]};
                    
                    auto bitangent_ozz = Cross(ozz::math::Float3(normal.x, normal.y, normal.z), ozz::math::Float3(tangent.x, tangent.y, tangent.z));
                    Vector3 bitangent = Vector3{bitangent_ozz.x, bitangent_ozz.y, bitangent_ozz.z};

                    std::array<uint16_t, 4> indices = {0, 0, 0, 0};
                    std::array<float, 3> weights = {0,0,0};
                    for (int k = 0; k < mesh->parts[i].influences_count(); k++)
                    {
                        indices[k] = mesh->parts[i].joint_indices[j*mesh->parts[i].influences_count()+k];
                        weights[k] = mesh->parts[i].joint_weights[j*(mesh->parts[i].influences_count()-1)+k];
                    }

                    vertices.push_back(VertexBoned{position, normal, tangent, bitangent, texCoord, indices, weights});
                }
            }

            
            return vertices;
        }

        std::vector<Index16> ProcessIndices(ozz::sample::Mesh* mesh)
        {
            std::vector<Index16> indices;
            
            ozz::vector<uint16_t> indices_ozz(mesh->triangle_indices.size());

            for (int i = 0; i < mesh->triangle_indices.size(); i++)
                indices_ozz[i] = mesh->triangle_indices[i];
            
            return indices;
        }

        DirectX::BoundingBox CalculateBoundingBox(const std::vector<VertexBoned>& vertices)
        {
            DirectX::BoundingBox aabb;
            DirectX::BoundingBox::CreateFromPoints(
                aabb,
                vertices.size(),
                reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices[0].Position),
                sizeof(VertexBoned));
            return aabb;
        }
    };
}
