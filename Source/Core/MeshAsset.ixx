export module MeshAsset;

import <vector>;
import <filesystem>;
import  <DirectXCollision.h>;

import AssetBase;
import Mesh;
import IRenderContext;
import RenderDevice;

namespace GiiGa
{
    export template <typename VertexType>
    class MeshAsset : public AssetBase, public Mesh<VertexType>
    {
    public:
        MeshAsset(
            AssetHandle handle,
            IRenderContext& render_context,
            RenderDevice& device, 
            const std::vector<VertexType>& vertices,
            const std::vector<Index16>& indices, 
            DirectX::BoundingBox aabb
        )
            : Mesh<VertexType>(render_context, device, vertices, indices, aabb)
            , AssetBase(handle)
        {
        }

        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;

        virtual AssetType GetType() override {
            return AssetType::Mesh;
        }
    };
}  // namespace GiiGa
