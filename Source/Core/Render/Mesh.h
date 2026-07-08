#pragma once

#include<span>
#include<vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<DirectXCollision.h>
#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<IRenderContext.h>
#include<VertexTypes.h>
#include<ObjectMask.h>

namespace GiiGa
{
    class RenderContext;

    template <typename VertexType>
    class Mesh
    {
    public:
        Mesh() = default;

        Mesh(IRenderContext& render_context, RenderDevice& device,
             const std::vector<VertexType>& vertices,
             const std::vector<Index16>& indices,
             DirectX::BoundingBox aabb):
            AABB(aabb)
        {
            {
                nvrhi::BufferDesc desc;
                desc.byteSize = vertices.size() * sizeof(VertexType);
                desc.isVertexBuffer = true;
                desc.initialState = nvrhi::ResourceStates::VertexBuffer;
                desc.keepInitialState = true;
                desc.debugName = "MeshVertexBuffer";
                vertexBuffer_ = device.NvDevice()->createBuffer(desc);
            }

            {
                nvrhi::BufferDesc desc;
                desc.byteSize = indices.size() * sizeof(Index16);
                desc.isIndexBuffer = true;
                desc.initialState = nvrhi::ResourceStates::IndexBuffer;
                desc.keepInitialState = true;
                desc.debugName = "MeshIndexBuffer";
                indexBuffer_ = device.NvDevice()->createBuffer(desc);
            }

            const auto vertices_span = std::span{reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size() * sizeof(VertexType)};
            render_context.WriteBufferDeffered(vertexBuffer_, vertices_span);

            const auto indices_span = std::span{reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(Index16)};
            render_context.WriteBufferDeffered(indexBuffer_, indices_span);

            indexCount = static_cast<uint32_t>(indices.size());
        }

        void Draw(RenderContext& context);

        DirectX::BoundingBox GetAABB() const
        {
            return AABB;
        }

        ObjectMask GetObjectMask() const
        {
            return vertex_type_mask_;
        }

    protected:
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::BufferHandle indexBuffer_;
        uint32_t indexCount = 0;
        DirectX::BoundingBox AABB;
        ObjectMask vertex_type_mask_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT);
    };
}

#include<RenderContext.h>

namespace GiiGa
{
    template <typename VertexType>
    void Mesh<VertexType>::Draw(RenderContext& context)
    {
        context.DrawIndexed(vertexBuffer_, indexBuffer_, Index16::Format, indexCount);
    }
}
