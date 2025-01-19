#pragma once
#include<DirectXCollision.h>

#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
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
#include<Misc.h>
#include<AssetHandle.h>

#include<ConcreteAsset/PrefabAsset.h>
#include<TransformComponent.h>
#include<StaticMeshComponent.h>
#include<GameObject.h>

#include<ImageAssetLoader.h>

#include<EditorAssetDatabase.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace GiiGa
{
    template <typename VertexType>
    class MeshAssetLoader : public AssetLoader
    {
    protected:

    public:
        virtual ~MeshAssetLoader() = default;

        MeshAssetLoader()
        {
            id_ = Uuid::FromString("ab702a1d-3b4f-4782-bac3-3af95b3145ac").value();
            pattern_ = R"((.+)\.(fbx|obj|gltf))";
            type_ = AssetType::Mesh;
        }

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
        {
            if (!std::filesystem::exists(absolute_path))
                throw std::runtime_error("File does not exist: " + absolute_path.string());

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(absolute_path.string(), aiProcess_Triangulate);

            if (!scene || !scene->HasMeshes())
            {
                throw std::runtime_error("Failed to load mesh from file: " + absolute_path.string());
            }

            auto materials_map = PreprocessMaterial(scene, scene->mNumMaterials, scene->mMaterials, absolute_path);

            std::unordered_map<AssetHandle, AssetMeta> handles;

            auto uuid = Uuid::New();
            auto go = CreateGameObjectHierarchy(
                scene->mRootNode,
                scene,
                uuid,
                relative_path,
                handles,
                materials_map
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

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path.string(),
                                                     aiProcess_Triangulate |
                                                     aiProcess_GenNormals |
                                                     aiProcess_JoinIdenticalVertices |
                                                     aiProcess_CalcTangentSpace |
                                                     aiProcess_ImproveCacheLocality);

            if (!scene || !scene->HasMeshes())
            {
                throw std::runtime_error("Failed to load mesh from file: " + path.string());
            }


            if (scene->mNumMeshes <= handle.subresource)
            {
                throw std::runtime_error("Wrong subresource");
            }

            aiMesh* mesh = scene->mMeshes[handle.subresource];
            if (!mesh)
            {
                throw std::runtime_error("No valid mesh found in file: " + path.string());
            }

            std::vector<VertexType> vertices = ProcessVertices(mesh);
            std::vector<Index16> indices = ProcessIndices(mesh);

            DirectX::BoundingBox aabb = CalculateBoundingBox(vertices);

            return std::make_shared<MeshAsset<VertexType>>(handle, rs->GetRenderContext(), rs->GetRenderDevice(), vertices, indices, aabb);
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
        Transform FromAiMatrix(const aiMatrix4x4& matrix)
        {
            aiVector3D scaling, position;
            aiQuaternion rotation;

            matrix.Decompose(scaling, rotation, position);

            return Transform(
                Vector3(position.x, position.y, position.z),
                Quaternion(rotation.x, rotation.y, rotation.z, rotation.w),
                Vector3(scaling.x, scaling.y, scaling.z)
            );
        }

        std::shared_ptr<GameObject> CreateGameObjectHierarchy(
            aiNode* node,
            const aiScene* scene,
            const Uuid& mesh_uuid,
            const std::filesystem::path& relative_path,
            std::unordered_map<AssetHandle, AssetMeta>& handles,
            const std::unordered_map<unsigned int, std::shared_ptr<Material>> materials_map
        )
        {
            auto gameObject = GameObject::CreateEmptyGameObject(SpawnParameters{
                node->mName.C_Str(),
                std::weak_ptr<IGameObject>(),
                std::weak_ptr<ILevelRootGameObjects>(),
            });

            aiMatrix4x4 transform = node->mTransformation;
            gameObject->GetTransformComponent().lock()->SetTransform(FromAiMatrix(transform));

            for (int i = 0; i < node->mNumMeshes; ++i)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                if (mesh)
                {
                    auto handle = AssetHandle{mesh_uuid, (int)node->mMeshes[i]};
                    // TODO: Handle skeletal
                    auto mesh_asset = std::make_shared<MeshAsset<VertexPNTBT>>(handle);

                    auto staticMeshComponent = gameObject->CreateComponent<StaticMeshComponent>();
                    staticMeshComponent->SetDummyMesh(mesh_asset);

                    handles.emplace(handle, AssetMeta{
                                        type_,
                                        relative_path,
                                        id_,
                                        mesh->mName.C_Str()
                                    });

                    if (materials_map.contains(mesh->mMaterialIndex))
                        staticMeshComponent->SetMaterial(materials_map.at(mesh->mMaterialIndex));
                }
            }

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
            {
                auto childGameObject = CreateGameObjectHierarchy(
                    node->mChildren[i],
                    scene,
                    mesh_uuid,
                    relative_path,
                    handles,
                    materials_map
                );
                gameObject->AddChild(childGameObject);
            }

            return gameObject;
        }

        std::unordered_map<unsigned int, std::shared_ptr<Material>> PreprocessMaterial(const aiScene* scene, unsigned int mat_num, aiMaterial** materials, std::filesystem::path absolute_path)
        {
            std::unordered_map<unsigned int, std::shared_ptr<Material>> materials_result;

            std::unordered_map<aiTextureType, std::shared_ptr<TextureAsset>> material_textures
            {
                {aiTextureType_BASE_COLOR, nullptr}
            };

            const std::unordered_map<aiTextureType, TexturesOrder> ai_tex_to_asset_texture_order
            {
                {aiTextureType_BASE_COLOR, TexturesOrder::BaseColor}
            };

            for (int i = 0; i < mat_num; ++i)
            {
                auto material = materials[i];

                if (material->GetName().length == 0)
                    continue;

                for (auto& [type, texture_asset] : material_textures)
                {
                    aiString text_path;
                    material->GetTexture(type, 0, &text_path);

                    if (text_path.length == 0)
                        continue;

                    if (auto embed_texture = scene->GetEmbeddedTexture(text_path.C_Str()))
                    {
                        Todo();
                    }
                    else
                    {
                        auto tex_handles = Engine::Instance().AssetDatabase()->GetHandlesByPath(text_path.C_Str());
                        if (!tex_handles.empty())
                        {
                            if (tex_handles.size() > 1)
                                el::Loggers::getLogger(LogResourceManager)->warn("Texture with same path (%v) has several handles", text_path.C_Str());

                            auto tex_handle = tex_handles[0];

                            texture_asset = Engine::Instance().ResourceManager()->GetAsset<TextureAsset>(std::get<AssetHandle>(tex_handle));
                        }
                        else
                        {
                            auto tex_path = absolute_path.remove_filename() / text_path.C_Str();
                            texture_asset = std::dynamic_pointer_cast<TextureAsset>(ImageAssetLoader().Load(AssetHandle{Uuid::New(), 0}, tex_path));
                            std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().AssetDatabase())->CreateAsset(texture_asset, tex_path, false);
                        }
                    }
                }

                std::string mat_name = material->GetName().C_Str();
                auto new_mat_asset = std::make_shared<Material>(mat_name);

                for (auto& [type, texture] : material_textures)
                {
                    if (texture)
                    {
                        new_mat_asset->SetTexture(ai_tex_to_asset_texture_order.at(type), texture);
                    }
                }

                aiColor4D color;
                material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                new_mat_asset->SetBaseColorTint({color.r, color.g, color.b});

                std::filesystem::path material_path = absolute_path.remove_filename() / (mat_name + ".mat");

                std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().AssetDatabase())->CreateAsset(new_mat_asset, material_path);

                materials_result.emplace(i, new_mat_asset);
            }

            return materials_result;
        }

        std::vector<VertexType> ProcessVertices(aiMesh* mesh)
        {
            if (mesh->HasBones())
            {
                // Можно использовать эту сладость: https://github.com/ColinGilbert/ozz-assimp-loader/tree/master
                Todo();
            }

            std::vector<VertexType> vertices(mesh->mNumVertices);

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
            {
                SimpleMath::Vector3 position{mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
                SimpleMath::Vector3 normal{0, 0, 0};
                SimpleMath::Vector2 texCoord{0, 0};
                SimpleMath::Vector3 tangent{0, 0, 0};
                SimpleMath::Vector3 bitangent{0, 0, 0};

                if (mesh->HasNormals())
                {
                    normal = Vector3{mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
                }

                if (mesh->HasTextureCoords(0))
                {
                    // flipping "y" coordinate
                    texCoord = Vector2{mesh->mTextureCoords[0][i].x, 1 - mesh->mTextureCoords[0][i].y};
                }

                if (mesh->HasTangentsAndBitangents())
                {
                    tangent = Vector3{mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
                    bitangent = Vector3{mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
                }

                if (typeid(VertexType) == typeid(VertexPosition))
                {
                    auto v = VertexPosition{position};
                    vertices[i] = *reinterpret_cast<VertexType*>(&v);
                }
                //vertices[i] = (VertexType{position});
                else if (typeid(VertexType) == typeid(VertexPNTBT))
                {
                    auto v = VertexPNTBT{position, normal, tangent, bitangent, texCoord};
                    vertices[i] = *reinterpret_cast<VertexType*>(&v);
                }
                /*else if (typeid(VertexType) == typeid(VertexBoned))
                    vertices[i] = VertexBoned(position, normal, texCoord, tangent, bitangent, .........);*/
            }

            return vertices;
        }

        std::vector<Index16> ProcessIndices(aiMesh* mesh)
        {
            std::vector<Index16> indices;
            indices.reserve(mesh->mNumFaces * 3);

            for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
            {
                const aiFace& face = mesh->mFaces[i];
                if (face.mNumIndices != 3)
                {
                    throw std::runtime_error("Non-triangular face detected in mesh.");
                }

                indices.push_back(static_cast<Index16>(face.mIndices[0]));
                indices.push_back(static_cast<Index16>(face.mIndices[1]));
                indices.push_back(static_cast<Index16>(face.mIndices[2]));
            }

            return indices;
        }

        DirectX::BoundingBox CalculateBoundingBox(const std::vector<VertexType>& vertices)
        {
            DirectX::BoundingBox aabb;
            DirectX::BoundingBox::CreateFromPoints(
                aabb,
                vertices.size(),
                reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices[0].Position),
                sizeof(VertexType));
            return aabb;
        }
    };
}
