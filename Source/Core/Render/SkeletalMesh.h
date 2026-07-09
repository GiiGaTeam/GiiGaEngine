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

        SkeletalMesh(RenderContext& render_context,
                     const std::vector<VertexType>& vertices,
                     const std::vector<Index16>& indices)
        {
            auto* device = render_context.GetDevice().Device();

            const size_t vbBytes = vertices.size() * sizeof(VertexType);
            const size_t ibBytes = indices.size() * sizeof(Index16);
            indexCount_ = static_cast<uint32_t>(indices.size());

            vertexBuffer_ = device->createBuffer(
                nvrhi::BufferDesc().setByteSize(vbBytes).setIsVertexBuffer(true)
                                   .setInitialState(nvrhi::ResourceStates::VertexBuffer).setKeepInitialState(true)
                                   .setDebugName("SkeletalVB"));

            indexBuffer_ = device->createBuffer(
                nvrhi::BufferDesc().setByteSize(ibBytes).setIsIndexBuffer(true)
                                   .setInitialState(nvrhi::ResourceStates::IndexBuffer).setKeepInitialState(true)
                                   .setDebugName("SkeletalIB"));

            nvrhi::CommandListHandle upload = device->createCommandList();
            upload->open();
            upload->writeBuffer(vertexBuffer_, vertices.data(), vbBytes);
            upload->writeBuffer(indexBuffer_, indices.data(), ibBytes);
            upload->close();
            nvrhi::ICommandList* list = upload;
            device->executeCommandLists(&list, 1);
        }

        void Bind(nvrhi::GraphicsState& state) const
        {
            state.addVertexBuffer(nvrhi::VertexBufferBinding{vertexBuffer_, 0, 0});
            state.setIndexBuffer(nvrhi::IndexBufferBinding{indexBuffer_, Index16::NvFormat, 0});
        }

        uint32_t GetIndexCount() const { return indexCount_; }

    private:
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::BufferHandle indexBuffer_;
        uint32_t indexCount_ = 0;
    };
}
