export module MeshAssetLoader;


import <d3d12.h>;
import <assimp/Importer.hpp>;
import <assimp/scene.h>;
import <assimp/postprocess.h>;
import <stdexcept>;
import <filesystem>;
import <memory>;
import <vector>;
import <DirectXCollision.h>;
import <directxtk/SimpleMath.h>;

import Uuid;
import AssetLoader;
import AssetType;
import MeshAsset;
import Engine;
import VertexTypes;

using namespace DirectX;

namespace GiiGa
{
    export class MeshAssetLoader : public AssetLoader
    {
    protected:

    public:
        MeshAssetLoader() {
            pattern_ = R"((.+)\.(fbx|obj|gltf))";
            type_ = AssetType::Mesh;
        }

        virtual std::vector<AssetHandle> Preprocess(const std::filesystem::path& path) {
            if (!std::filesystem::exists(path))
                throw std::runtime_error("File does not exist: " + path.string());

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path.string(), 0);

            if (!scene || !scene->HasMeshes()) {
                throw std::runtime_error("Failed to load mesh from file: " + path.string());
            }

            std::vector<AssetHandle> handles;

            auto uuid = Uuid::New();
            for (int i = 0; i < scene->mNumMeshes; ++i) {
                handles.push_back(AssetHandle(uuid, i));
            }

            return handles;
        }

        std::shared_ptr<AssetBase> Load(AssetHandle handle, const std::filesystem::path& path) override {
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

            if (!scene || !scene->HasMeshes()) {
                throw std::runtime_error("Failed to load mesh from file: " + path.string());
            }

            
            if (scene->mNumMeshes <= handle.subresource) {
                throw std::runtime_error("Wrong subresource");
            }

            aiMesh* mesh = scene->mMeshes[handle.subresource];
            if (!mesh) {
                throw std::runtime_error("No valid mesh found in file: " + path.string());
            }

            std::vector<VertexPNTBT> vertices = ProcessVertices(mesh);
            std::vector<Index16> indices = ProcessIndices(mesh);

            DirectX::BoundingBox aabb = CalculateBoundingBox(vertices);

             return std::make_shared<MeshAsset>(handle, rs->GetRenderContext(), rs->GetRenderDevice(), vertices, indices, aabb);
        }

        void Save(AssetBase& asset, std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving Mesh is not supported.");
        }

        const char* GetName() override {
            return "Mesh Loader";
        }
    private:
        std::vector<VertexPNTBT> ProcessVertices(aiMesh* mesh) {
            std::vector<VertexPNTBT> vertices(mesh->mNumVertices);

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                SimpleMath::Vector3 position{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                SimpleMath::Vector3 normal{ 0, 0, 0 };
                SimpleMath::Vector3 texCoord{ 0, 0, 0 };
                SimpleMath::Vector3 tangent{ 0, 0, 0 };
                SimpleMath::Vector3 bitangent{ 0, 0, 0 };

                if (mesh->HasNormals()) {
                    normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
                }

                if (mesh->HasTextureCoords(0)) {
                    texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, 0.0f };
                }

                if (mesh->HasTangentsAndBitangents()) {
                    tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                    bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
                }

                vertices[i] = VertexPNTBT(position, normal, texCoord, tangent, bitangent);
            }

            return vertices;
        }

        std::vector<Index16> ProcessIndices(aiMesh* mesh) {
            std::vector<Index16> indices;
            indices.reserve(mesh->mNumFaces * 3);

            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace& face = mesh->mFaces[i];
                if (face.mNumIndices != 3) {
                    throw std::runtime_error("Non-triangular face detected in mesh.");
                }

                indices.push_back(static_cast<Index16>(face.mIndices[0]));
                indices.push_back(static_cast<Index16>(face.mIndices[1]));
                indices.push_back(static_cast<Index16>(face.mIndices[2]));
            }

            return indices;
        }

        DirectX::BoundingBox CalculateBoundingBox(const std::vector<VertexPNTBT>& vertices) {
            DirectX::BoundingBox aabb;
            DirectX::BoundingBox::CreateFromPoints(
                aabb,
                vertices.size(),
                reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices[0].Position),
                sizeof(VertexPNTBT));
            return aabb;
        }
    };
}
