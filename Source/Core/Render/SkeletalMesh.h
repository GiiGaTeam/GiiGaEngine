#pragma once


#include<span>
#include<vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directx/d3dx12.h>
#include<directxtk12/SimpleMath.h>

#include<RenderDevice.h>
#include<IRenderContext.h>
#include<GPULocalResource.h>
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
                     const std::vector<Index16>& indices):
            vertexBuffer_(std::make_shared<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(VertexType), D3D12_RESOURCE_FLAG_NONE))),
            indexBuffer_(std::make_shared<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(Index16), D3D12_RESOURCE_FLAG_NONE)))
        {
            const auto vertices_span = std::span{reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size() * sizeof(VertexType)};
            vertexBuffer_->UpdateContentsDeffered(render_context, vertices_span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            vertexView_ = vertexBuffer_->EmplaceVetexBufferView(
                D3D12_VERTEX_BUFFER_VIEW{0, static_cast<UINT>(vertices.size() * sizeof(VertexType)), sizeof(VertexType)});
            
            auto indices_span = std::span{reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(Index16)};
            indexCount = static_cast<UINT>(indices.size());
            indexBuffer_->UpdateContentsDeffered(render_context, indices_span, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            indexView_ = indexBuffer_->EmplaceIndexBufferView(
                D3D12_INDEX_BUFFER_VIEW{0, static_cast<UINT>(indices.size() * sizeof(Index16)), Index16::Format});
        }

        void Draw(const std::shared_ptr<ID3D12GraphicsCommandList>& cmd_list)
        {
            auto vertextDesc = vertexView_->getDescriptor();
            cmd_list->IASetVertexBuffers(0, 1, &vertextDesc);

            auto indexDesc = indexView_->getDescriptor();
            cmd_list->IASetIndexBuffer(&indexDesc);

            cmd_list->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        }

    private:
        std::shared_ptr<GPULocalResource> vertexBuffer_;
        std::shared_ptr<BufferView<Vertex>> vertexView_;
        UINT indexCount;
        std::shared_ptr<GPULocalResource> indexBuffer_;
        std::shared_ptr<BufferView<Index>> indexView_;
    };
}
