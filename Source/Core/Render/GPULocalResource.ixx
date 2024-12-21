export module GPULocalResource;

import <memory>;
import<span>;
import<algorithm>;
import<unordered_map>;
import<directx/d3dx12.h>;

import RenderDevice;
import IRenderContext;
import UploadBuffer;
export import BufferView;
export import DirectXUtils;

namespace GiiGa
{
    export class GPULocalResource : public std::enable_shared_from_this<GPULocalResource>
    {
    public:
        GPULocalResource(RenderDevice& device, std::shared_ptr<ID3D12Resource> resource,
                         D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON)
            : device_(device)
              , resource_(resource),
              current_state_(initialState)
        {
        }

        GPULocalResource(RenderDevice& device, D3D12_RESOURCE_DESC desc,
                         D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST, D3D12_CLEAR_VALUE* clearValue = nullptr):
            device_(device),
            current_state_(initialState)
        {
            CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            resource_ = device.CreateCommittedResource(heapProperties, D3D12_HEAP_FLAG_NONE, desc, current_state_, clearValue);
        }

        // Deffered occurs only in beginning of frame
        // should **not** be called after frame start
        // instead call UpdateContentsImmediate
        void UpdateContentsDeffered(IRenderContext& render_context, const std::span<const uint8_t>& data, D3D12_RESOURCE_STATES stateAfter)
        {
            auto s_this = shared_from_this();
            render_context.CreateDefferedUpload(data,
                                                {
                                                    .getStateFunction = [s_this]() { return s_this->current_state_; },
                                                    .setStateFunction = [s_this](D3D12_RESOURCE_STATES new_state) { return s_this->current_state_ = new_state; },
                                                    .stateAfter = stateAfter,
                                                    .destination = resource_
                                                }
            );
        }

        // If you do want to update contents after frame start use this
        void UpdateContentsImmediate(IRenderContext& render_context, const std::span<const uint8_t>& data, D3D12_RESOURCE_STATES stateAfer)
        {
            UploadBuffer::Allocation allocation = render_context.CreateAndAllocateUploadBufferInCurrentFrame(data.size());

            std::copy(data.begin(), data.end(), allocation.CPU.begin());

            StateTransition(render_context, D3D12_RESOURCE_STATE_COPY_DEST);

            render_context.CopyBufferRegion(resource_, 0, allocation.resource, 0, data.size());

            StateTransition(render_context, stateAfer);
        }

        void StateTransition(IRenderContext& render_context, D3D12_RESOURCE_STATES stateAfer)
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource_.get(),
                current_state_,
                stateAfer
            );

            render_context.ResourceBarrier(1, barrier);

            current_state_ = stateAfer;
        }

        std::shared_ptr<ID3D12Resource> GetResource()
        {
            return resource_;
        }

        ///////////////////////// GET BY DESC /////////////////////////////////////////////////

        std::shared_ptr<BufferView<Constant>> GetViewByDesc(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
        {
            return constantViews_.at(desc);
        }

        std::shared_ptr<BufferView<ShaderResource>> GetViewByDesc(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
        {
            return shaderResourceViews_.at(desc);
        }

        std::shared_ptr<BufferView<UnorderedAccess>> GetViewByDesc(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
        {
            return unorderedAccessViews_.at(desc);
        }

        std::shared_ptr<BufferView<RenderTarget>> GetViewByDesc(const D3D12_RENDER_TARGET_VIEW_DESC& desc)
        {
            return renderTargetViews_.at(desc);
        }

        std::shared_ptr<BufferView<DepthStencil>> GetViewByDesc(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
        {
            return depthStencilViews_.at(desc);
        }

        std::shared_ptr<BufferView<Index>> GetViewByDesc(const D3D12_INDEX_BUFFER_VIEW& desc)
        {
            return indexViews_.at(desc);
        }

        std::shared_ptr<BufferView<Vertex>> GetViewByDesc(const D3D12_VERTEX_BUFFER_VIEW& desc)
        {
            return vertexViews_.at(desc);
        }

        ///////////////////////// GET FIRST /////////////////////////////////////////////////

        std::shared_ptr<BufferView<Constant>> GetViewFirstConstant()
        {
            return constantViews_.begin()->second;
        }

        std::shared_ptr<BufferView<ShaderResource>> GetViewFirstShaderResource()
        {
            return shaderResourceViews_.begin()->second;
        }

        std::shared_ptr<BufferView<UnorderedAccess>> GetViewFirstUnorderedAccess()
        {
            return unorderedAccessViews_.begin()->second;
        }

        std::shared_ptr<BufferView<RenderTarget>> GetViewFirstRenderTarget()
        {
            return renderTargetViews_.begin()->second;
        }

        std::shared_ptr<BufferView<DepthStencil>> GetViewFirstDepthStencil()
        {
            return depthStencilViews_.begin()->second;
        }

        std::shared_ptr<BufferView<Index>> GetViewFirstIndex()
        {
            return indexViews_.begin()->second;
        }

        std::shared_ptr<BufferView<Vertex>> GetViewFirstVertex()
        {
            return vertexViews_.begin()->second;
        }

        ///////////////////////// CREATE VIEW /////////////////////////////////////////////////

        std::shared_ptr<BufferView<Constant>> CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC key = desc;
            desc.BufferLocation = resource_->GetGPUVirtualAddress();
            key.BufferLocation = 0;

            if (!constantViews_.contains(key))
                constantViews_.emplace(key, device_.CreateConstantBufferView(desc));

            return constantViews_[key];
        }

        std::shared_ptr<BufferView<ShaderResource>> EmplaceShaderResourceBufferView(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
        {
            if (!shaderResourceViews_.contains(desc))
                shaderResourceViews_.emplace(desc, device_.CreateShaderResourceBufferView(resource_, desc));

            return shaderResourceViews_[desc];
        }

        std::shared_ptr<BufferView<UnorderedAccess>> EmplaceUnorderedAccessView(const std::shared_ptr<ID3D12Resource>& counterBuffer,
                                                                                const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
        {
            if (!unorderedAccessViews_.contains(desc))
                unorderedAccessViews_.emplace(desc, device_.CreateUnorderedAccessView(resource_, counterBuffer, desc));

            return unorderedAccessViews_[desc];
        }

        std::shared_ptr<BufferView<RenderTarget>> EmplaceRenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC* desc)
        {
            D3D12_RENDER_TARGET_VIEW_DESC temp_copy{};
            if (desc != nullptr)
                temp_copy = *desc;

            if (!renderTargetViews_.contains(temp_copy))
                renderTargetViews_.emplace(temp_copy, device_.CreateRenderTargetView(resource_, desc));

            return renderTargetViews_[temp_copy];
        }

        std::shared_ptr<BufferView<DepthStencil>> EmplaceDepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
        {
            if (!depthStencilViews_.contains(desc))
                depthStencilViews_.emplace(desc, device_.CreateDepthStencilView(resource_, desc));

            return depthStencilViews_[desc];
        }

        std::shared_ptr<BufferView<Index>> EmplaceIndexBufferView(D3D12_INDEX_BUFFER_VIEW desc)
        {
            D3D12_INDEX_BUFFER_VIEW key = desc;
            desc.BufferLocation = resource_->GetGPUVirtualAddress();
            key.BufferLocation = 0;

            if (!indexViews_.contains(key))
                indexViews_.emplace(key, device_.CreateIndexBufferView(resource_, desc));

            return indexViews_[key];
        }

        std::shared_ptr<BufferView<Vertex>> EmplaceVetexBufferView(D3D12_VERTEX_BUFFER_VIEW desc)
        {
            D3D12_VERTEX_BUFFER_VIEW key = desc;
            desc.BufferLocation = resource_->GetGPUVirtualAddress();
            key.BufferLocation = 0;

            if (!vertexViews_.contains(key))
                vertexViews_.emplace(key, device_.CreateVetexBufferView(resource_, desc));

            return vertexViews_[key];
        }

    private:
        RenderDevice& device_;
        std::shared_ptr<ID3D12Resource> resource_;
        D3D12_RESOURCE_STATES current_state_;
        std::unordered_map<D3D12_CONSTANT_BUFFER_VIEW_DESC, std::shared_ptr<BufferView<Constant>>> constantViews_;
        std::unordered_map<D3D12_SHADER_RESOURCE_VIEW_DESC, std::shared_ptr<BufferView<ShaderResource>>> shaderResourceViews_;
        std::unordered_map<D3D12_UNORDERED_ACCESS_VIEW_DESC, std::shared_ptr<BufferView<UnorderedAccess>>> unorderedAccessViews_;
        std::unordered_map<D3D12_RENDER_TARGET_VIEW_DESC, std::shared_ptr<BufferView<RenderTarget>>> renderTargetViews_;
        std::unordered_map<D3D12_DEPTH_STENCIL_VIEW_DESC, std::shared_ptr<BufferView<DepthStencil>>> depthStencilViews_;
        std::unordered_map<D3D12_INDEX_BUFFER_VIEW, std::shared_ptr<BufferView<Index>>> indexViews_;
        std::unordered_map<D3D12_VERTEX_BUFFER_VIEW, std::shared_ptr<BufferView<Vertex>>> vertexViews_;
    };
}
