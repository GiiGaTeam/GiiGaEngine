#pragma once

// MeshAsset.h — templated mesh turned into an engine asset.
// NVRHI cutover: the constructor takes the nvrhi device + an open upload command list
// (the loader opens, constructs, then closes/executes/waits).

#include <DirectXCollision.h>
#include <vector>

#include <AssetBase.h>
#include <Mesh.h>
#include <Core/Render/Nvrhi/NvrhiResources.h>

namespace GiiGa
{
    template <typename VertexType>
    class MeshAsset : public AssetBase, public Mesh<VertexType>
    {
    public:
        MeshAsset(AssetHandle handle,
                  nvrhi::IDevice* device,
                  nvrhi::ICommandList* uploadCl,
                  const std::vector<VertexType>& vertices,
                  const std::vector<Index16>& indices,
                  DirectX::BoundingBox aabb)
            : Mesh<VertexType>(device, uploadCl, vertices, indices, aabb)
            , AssetBase(handle)
        {
        }

        explicit MeshAsset(AssetHandle handle)
            : AssetBase(handle)
        {
        }

        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;

        virtual AssetType GetType() override
        {
            return AssetType::Mesh;
        }
    };
}
