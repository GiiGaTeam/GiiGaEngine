#pragma once


#include <DirectXCollision.h>



#include<vector>;

#include<AssetBase.h>
#include<Mesh.h>
#include<RenderContext.h>

namespace GiiGa
{
    template <typename VertexType>
    class MeshAsset : public AssetBase, public Mesh<VertexType>
    {
    public:
        MeshAsset(
            AssetHandle handle,
            RenderContext& render_context,
            const std::vector<VertexType>& vertices,
            const std::vector<Index16>& indices,
            DirectX::BoundingBox aabb
        )
            : Mesh<VertexType>(render_context, vertices, indices, aabb)
            , AssetBase(handle)
        {
        }

        MeshAsset(
            AssetHandle handle
        )
            : AssetBase(handle)
        {
        }

        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;

        virtual AssetType GetType() override {
            return AssetType::Mesh;
        }
    };
}  // namespace GiiGa
