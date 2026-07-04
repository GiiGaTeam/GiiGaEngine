#pragma once

// Mesh.h — templated static mesh holding NVRHI vertex/index buffers.
// NVRHI cutover: no more GPULocalResource / BufferView / D3D12 views; just two
// nvrhi::BufferHandle (created with keepInitialState so the frame command list can use them
// without explicit state tracking) plus the AABB and vertex-type mask. Drawing is now done
// by the render pass (which assembles GraphicsState), so the old Draw() is gone.

#include <vector>
#include <memory>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <DirectXCollision.h>

#include <Core/Render/Nvrhi/NvrhiResources.h>   // createVertexBuffer / createIndexBuffer + nvrhi + VertexTypes
#include <ObjectMask.h>

namespace GiiGa
{
    template <typename VertexType>
    class Mesh
    {
    public:
        Mesh() = default;

        Mesh(nvrhi::IDevice* device,
             nvrhi::ICommandList* uploadCl,
             const std::vector<VertexType>& vertices,
             const std::vector<Index16>& indices,
             DirectX::BoundingBox aabb)
            : aabb_(aabb)
            , indexCount(static_cast<uint32_t>(indices.size()))
        {
            vertexBuffer_ = rhi::createVertexBuffer<VertexType>(device, uploadCl, vertices, "mesh vb");
            indexBuffer_ = rhi::createIndexBuffer<Index16>(device, uploadCl, indices, nvrhi::Format::R16_UINT, "mesh ib");
        }

        nvrhi::IBuffer* GetVertexBuffer() const { return vertexBuffer_.Get(); }
        nvrhi::IBuffer* GetIndexBuffer() const { return indexBuffer_.Get(); }
        uint32_t GetIndexCount() const { return indexCount; }
        nvrhi::Format GetIndexFormat() const { return nvrhi::Format::R16_UINT; }

        DirectX::BoundingBox GetAABB() const { return aabb_; }
        ObjectMask GetObjectMask() const { return vertex_type_mask_; }

    protected:
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::BufferHandle indexBuffer_;
        uint32_t indexCount = 0;
        DirectX::BoundingBox aabb_;
        ObjectMask vertex_type_mask_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT);
    };
}
