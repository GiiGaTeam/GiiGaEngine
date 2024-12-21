export module Mesh;

import <DirectXCollision.h>;
import <span>;
import <vector>;
import<directx/d3dx12.h>;

import AssetBase;
import RenderDevice;
import IRenderContext;
import GPULocalResource;
export import VertexTypes;
export import ObjectMask;

namespace GiiGa
{
    export class Mesh
    {
    public:
        using VertexType = VertexPNTBT;

        Mesh(IRenderContext& render_context, RenderDevice& device,
             const std::vector<VertexType>& vertices,
             const std::vector<Index16>& indices,
             DirectX::BoundingBox aabb):
            vertexBuffer_(std::make_shared<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(VertexType), D3D12_RESOURCE_FLAG_NONE), D3D12_RESOURCE_STATE_COMMON)),
            indexBuffer_(std::make_shared<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(Index16), D3D12_RESOURCE_FLAG_NONE), D3D12_RESOURCE_STATE_COMMON)),
            AABB(aabb)
        {
            // do we want to save vertices and indices for later use?

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

        DirectX::BoundingBox GetAABB() const
        {
            return AABB;
        }

        ObjectMask GetObjectMask() const
        {
            return vertex_type_mask_;
        }

    private:
        std::shared_ptr<GPULocalResource> vertexBuffer_;
        std::shared_ptr<BufferView<Vertex>> vertexView_;
        UINT indexCount;
        std::shared_ptr<GPULocalResource> indexBuffer_;
        std::shared_ptr<BufferView<Index>> indexView_;
        DirectX::BoundingBox AABB;
        ObjectMask vertex_type_mask_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT);
    };
}
