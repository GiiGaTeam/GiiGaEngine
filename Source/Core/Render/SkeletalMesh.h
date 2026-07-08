#pragma once


#include<span>
#include<vector>
#include<string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<IRenderContext.h>
#include<RenderContext.h>
#include<VertexTypes.h>

using namespace DirectX;

namespace GiiGa
{
    struct Joint
    {
        SimpleMath::Matrix invBindPose;
        std::string name;
        uint16_t parentBone;
    };

    using Skeleton = std::vector<Joint>;

    class SkeletalMesh
    {
    public:
        using VertexType = VertexBoned;

        SkeletalMesh(IRenderContext& render_context, RenderDevice& device,
                     const std::vector<VertexType>& vertices,
                     const std::vector<Index16>& indices)
        {
            {
                nvrhi::BufferDesc desc;
                desc.byteSize = vertices.size() * sizeof(VertexType);
                desc.isVertexBuffer = true;
                desc.initialState = nvrhi::ResourceStates::VertexBuffer;
                desc.keepInitialState = true;
                desc.debugName = "SkeletalMeshVertexBuffer";
                vertexBuffer_ = device.NvDevice()->createBuffer(desc);
            }

            {
                nvrhi::BufferDesc desc;
                desc.byteSize = indices.size() * sizeof(Index16);
                desc.isIndexBuffer = true;
                desc.initialState = nvrhi::ResourceStates::IndexBuffer;
                desc.keepInitialState = true;
                desc.debugName = "SkeletalMeshIndexBuffer";
                indexBuffer_ = device.NvDevice()->createBuffer(desc);
            }

            const auto vertices_span = std::span{reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size() * sizeof(VertexType)};
            render_context.WriteBufferDeffered(vertexBuffer_, vertices_span);

            auto indices_span = std::span{reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(Index16)};
            render_context.WriteBufferDeffered(indexBuffer_, indices_span);

            indexCount = static_cast<uint32_t>(indices.size());
        }

        void Draw(RenderContext& context)
        {
            context.DrawIndexed(vertexBuffer_, indexBuffer_, Index16::Format, indexCount);
        }

    private:
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::BufferHandle indexBuffer_;
        uint32_t indexCount = 0;
    };
}
