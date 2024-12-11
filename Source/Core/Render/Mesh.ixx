module;

#include <DirectXCollision.h>
#include <span>
#include <vector>
#include<directx/d3dx12.h>

export module Mesh;

import AssetBase;
import RenderDevice;
import IRenderContext;
import GPULocalResource;
export import VertexTypes;

namespace GiiGa
{
    export class Mesh : public AssetBase
    {
    public:
        using VertexType = VertexPNTBT;

        Mesh(Uuid id, RenderDevice& device, const std::vector<VertexType>& vertices,
             const std::vector<Index16>& indices, DirectX::BoundingBox aabb):
            AssetBase(AssetHandle{id, AssetType::Mesh}),
            vertexBuffer_(device, CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(VertexType), D3D12_RESOURCE_FLAG_NONE)),
            indexBuffer_(device, CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(Index16), D3D12_RESOURCE_FLAG_NONE)),
            AABB(aabb)
        {
            throw std::exception();
        }

        Mesh(IRenderContext& render_context, RenderDevice& device, const std::vector<VertexType>& vertices,
             const std::vector<Index16>& indices, DirectX::BoundingBox aabb):
            AssetBase(AssetHandle{Uuid::New(), AssetType::Mesh}),
            vertexBuffer_(device, CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(VertexType), D3D12_RESOURCE_FLAG_NONE)),
            indexBuffer_(device, CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(Index16), D3D12_RESOURCE_FLAG_NONE)),
            AABB(aabb)
        {
            const auto vertices_span = std::span{reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size() * sizeof(VertexType)};
            vertexBuffer_.UpdateContents(render_context, vertices_span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            vertexView_ = vertexBuffer_.CreateVetexBufferView(
                D3D12_VERTEX_BUFFER_VIEW{0, static_cast<UINT>(vertices.size() * sizeof(VertexType)), sizeof(VertexType)});

            indexCount = static_cast<UINT>(indices.size());
            auto indices_span = std::span{reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(Index16)};
            indexBuffer_.UpdateContents(render_context, indices_span, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            indexView_ = indexBuffer_.CreateIndexBufferView(
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

        DirectX::BoundingBox GetAABB() const
        {
            return AABB;
        }

    private:
        GPULocalResource vertexBuffer_;
        std::shared_ptr<BufferView<Vertex>> vertexView_;
        UINT indexCount;
        GPULocalResource indexBuffer_;
        std::shared_ptr<BufferView<Index>> indexView_;
        DirectX::BoundingBox AABB;
    };
}
