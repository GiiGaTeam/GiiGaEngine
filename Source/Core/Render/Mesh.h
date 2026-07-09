#pragma once

#include<span>
#include<vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<DirectXCollision.h>

#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<RenderContext.h>
#include<VertexTypes.h>
#include<ObjectMask.h>

namespace GiiGa
{
    template <typename VertexType>
    class Mesh
    {
    public:
        Mesh() = default;

        Mesh(RenderContext& render_context,
             const std::vector<VertexType>& vertices,
             const std::vector<Index16>& indices,
             DirectX::BoundingBox aabb):
            AABB(aabb)
        {
            auto* device = render_context.GetDevice().Device();

            const size_t vbBytes = vertices.size() * sizeof(VertexType);
            const size_t ibBytes = indices.size() * sizeof(Index16);
            indexCount_ = static_cast<uint32_t>(indices.size());

            vertexBuffer_ = device->createBuffer(
                nvrhi::BufferDesc().setByteSize(vbBytes).setIsVertexBuffer(true)
                                   .setInitialState(nvrhi::ResourceStates::VertexBuffer).setKeepInitialState(true)
                                   .setDebugName("MeshVB"));

            indexBuffer_ = device->createBuffer(
                nvrhi::BufferDesc().setByteSize(ibBytes).setIsIndexBuffer(true)
                                   .setInitialState(nvrhi::ResourceStates::IndexBuffer).setKeepInitialState(true)
                                   .setDebugName("MeshIB"));

            // Meshes are created at asset-load time (outside the render frame), so upload
            // through a dedicated one-shot command list rather than the frame command list.
            nvrhi::CommandListHandle upload = device->createCommandList();
            upload->open();
            upload->writeBuffer(vertexBuffer_, vertices.data(), vbBytes);
            upload->writeBuffer(indexBuffer_, indices.data(), ibBytes);
            upload->close();
            nvrhi::ICommandList* list = upload;
            device->executeCommandLists(&list, 1);
        }

        // Adds this mesh's vertex/index buffers to a graphics state being assembled by a pass.
        void Bind(nvrhi::GraphicsState& state) const
        {
            state.addVertexBuffer(nvrhi::VertexBufferBinding{vertexBuffer_, 0, 0});
            state.setIndexBuffer(nvrhi::IndexBufferBinding{indexBuffer_, Index16::NvFormat, 0});
        }

        uint32_t GetIndexCount() const { return indexCount_; }

        DirectX::BoundingBox GetAABB() const { return AABB; }

        ObjectMask GetObjectMask() const { return vertex_type_mask_; }

    protected:
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::BufferHandle indexBuffer_;
        uint32_t indexCount_ = 0;
        DirectX::BoundingBox AABB;
        ObjectMask vertex_type_mask_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT);
    };
}
