module;

#include <vector>
#include <filesystem>

export module MeshAsset;

import AssetBase;
import Mesh;

namespace GiiGa
{
    export class MeshAsset : public AssetBase, public Mesh
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
            : Mesh(render_context, device, vertices, indices, aabb)
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
