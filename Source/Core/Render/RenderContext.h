#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>

#include<memory>
#include<vector>
#include<queue>
#include<mutex>
#include<unordered_map>
#include<algorithm>
#include<cassert>
#include<tracy/tracy/Tracy.hpp>
#include<tracy/tracy/TracyD3D12.hpp>

#include<IRenderContext.h>
#include<RenderSystemSettings.h>
#include<RenderDevice.h>
#include<NvrhiUtils.h>

namespace GiiGa
{
    // The frame-recording context: owns the graphics nvrhi command list,
    // frame pacing (swap-chain latency waitable), a deferred-upload queue for
    // resources created outside of a frame, a binding-set cache, and the
    // current graphics state (pipeline/framebuffer/viewport/bindings) that
    // drawables are issued against.
    class RenderContext : public IRenderContext
    {
        friend class EditorSwapChainPass;

        struct DefferedUploadEntry
        {
            nvrhi::BufferHandle buffer;
            std::vector<uint8_t> data;
        };

    public:
        explicit RenderContext(RenderDevice& device):
            device_(device)
        {
            command_list_ = device_.NvDevice()->createCommandList();
        }

        void SetFrameLatencyWaitableObject(HANDLE FrameLatencyWaitableObject)
        {
            frameLatencyWaitableObject_ = FrameLatencyWaitableObject;
        }

        ///////////////////////// UPLOADS /////////////////////////////////////////////////

        void WriteBufferDeffered(nvrhi::BufferHandle buffer, const std::span<const uint8_t>& data) override
        {
            std::lock_guard lock(deffered_uploads_mutex_);
            defferedUploads_.push(DefferedUploadEntry{buffer, std::vector<uint8_t>(data.begin(), data.end())});
        }

        void WriteBufferImmediate(nvrhi::IBuffer* buffer, const std::span<const uint8_t>& data) override
        {
            assert(is_recording_);
            command_list_->writeBuffer(buffer, data.data(), data.size());
        }

        nvrhi::ICommandList* CommandList() override
        {
            return command_list_;
        }

        ID3D12GraphicsCommandList* GetNativeCommandList()
        {
            return static_cast<ID3D12GraphicsCommandList*>(
                command_list_->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList).pointer);
        }

        ///////////////////////// FRAME LOOP /////////////////////////////////////////////////

        void StartFrame()
        {
            ZoneScoped;
            if (frameLatencyWaitableObject_)
            {
                ZoneScopedN("frameLatencyWaitableObject");
                WaitForSingleObject(frameLatencyWaitableObject_, INFINITE);
            }

            command_list_->open();
            is_recording_ = true;

            TracyD3D12NewFrame(device_.GetTracyGpuContext());

            {
                std::lock_guard lock(deffered_uploads_mutex_);
                while (!defferedUploads_.empty())
                {
                    DefferedUploadEntry entry = std::move(defferedUploads_.front());
                    defferedUploads_.pop();
                    command_list_->writeBuffer(entry.buffer, entry.data.data(), entry.data.size());
                }
            }
        }

        void EndFrame()
        {
            ZoneScoped;
            command_list_->close();
            is_recording_ = false;
            device_.NvDevice()->executeCommandList(command_list_);
            device_.NvDevice()->runGarbageCollection();
            TracyD3D12Collect(device_.GetTracyGpuContext());
        }

        void ContextIdle()
        {
            device_.NvDevice()->waitForIdle();
            device_.NvDevice()->runGarbageCollection();
        }

        ///////////////////////// GRAPHICS STATE /////////////////////////////////////////////////

        void SetPipeline(nvrhi::IGraphicsPipeline* pipeline)
        {
            current_pipeline_ = pipeline;
        }

        void SetFramebuffer(nvrhi::IFramebuffer* framebuffer)
        {
            current_framebuffer_ = framebuffer;
        }

        void SetViewport(float width, float height)
        {
            current_viewport_ = nvrhi::Viewport(width, height);
        }

        void SetBindingSet(nvrhi::BindingSetHandle binding_set)
        {
            current_binding_set_ = binding_set;
        }

        nvrhi::IFramebuffer* GetFramebuffer() const { return current_framebuffer_; }
        nvrhi::Viewport GetViewport() const { return current_viewport_; }

        void DrawIndexed(nvrhi::IBuffer* vertex_buffer, nvrhi::IBuffer* index_buffer, nvrhi::Format index_format, uint32_t index_count)
        {
            ZoneScoped;
            nvrhi::GraphicsState state;
            state.pipeline = current_pipeline_;
            state.framebuffer = current_framebuffer_;
            state.viewport.addViewportAndScissorRect(current_viewport_);
            if (current_binding_set_)
                state.bindings = {current_binding_set_};
            state.vertexBuffers = {nvrhi::VertexBufferBinding{vertex_buffer, 0, 0}};
            state.indexBuffer = nvrhi::IndexBufferBinding{index_buffer, index_format, 0};

            command_list_->setGraphicsState(state);

            nvrhi::DrawArguments args;
            args.vertexCount = index_count;
            args.instanceCount = 1;
            command_list_->drawIndexed(args);
        }

        ///////////////////////// BINDING SET CACHE /////////////////////////////////////////////////

        nvrhi::BindingSetHandle GetOrCreateBindingSet(nvrhi::BindingSetDesc desc, nvrhi::IBindingLayout* layout)
        {
            ZoneScoped;
            // Canonical order (matches how PSO builds its layout) so that
            // logically-equal descs hash equally and match layout item order.
            std::sort(desc.bindings.begin(), desc.bindings.end(),
                      [](const nvrhi::BindingSetItem& a, const nvrhi::BindingSetItem& b)
                      {
                          if (a.type != b.type)
                              return static_cast<int>(a.type) < static_cast<int>(b.type);
                          return a.slot < b.slot;
                      });

            size_t hash = HashBindingSetDesc(desc, layout);

            auto [begin, end] = binding_cache_.equal_range(hash);
            for (auto it = begin; it != end; ++it)
            {
                if (BindingSetDescsEqual(it->second.first, desc) && it->second.second->getLayout() == layout)
                    return it->second.second;
            }

            nvrhi::BindingSetHandle binding_set = device_.NvDevice()->createBindingSet(desc, layout);
            binding_cache_.emplace(hash, std::make_pair(desc, binding_set));
            return binding_set;
        }

        void ClearBindingCache()
        {
            binding_cache_.clear();
        }

        RenderDevice& GetDevice() { return device_; }

    private:
        RenderDevice& device_;
        nvrhi::CommandListHandle command_list_;
        bool is_recording_ = false;

        HANDLE frameLatencyWaitableObject_ = nullptr;

        std::mutex deffered_uploads_mutex_;
        std::queue<DefferedUploadEntry> defferedUploads_;

        nvrhi::IGraphicsPipeline* current_pipeline_ = nullptr;
        nvrhi::IFramebuffer* current_framebuffer_ = nullptr;
        nvrhi::Viewport current_viewport_ = nvrhi::Viewport(300, 300);
        nvrhi::BindingSetHandle current_binding_set_;

        std::unordered_multimap<size_t, std::pair<nvrhi::BindingSetDesc, nvrhi::BindingSetHandle>> binding_cache_;

        static void HashCombine(size_t& seed, size_t value)
        {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        static size_t HashBindingSetDesc(const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout)
        {
            size_t seed = std::hash<void*>()(layout);
            for (const auto& item : desc.bindings)
            {
                HashCombine(seed, std::hash<void*>()(item.resourceHandle));
                HashCombine(seed, std::hash<uint32_t>()(item.slot));
                HashCombine(seed, std::hash<int>()(static_cast<int>(item.type)));
                HashCombine(seed, std::hash<int>()(static_cast<int>(item.dimension)));
                HashCombine(seed, std::hash<int>()(static_cast<int>(item.format)));
            }
            return seed;
        }

        static bool BindingSetDescsEqual(const nvrhi::BindingSetDesc& a, const nvrhi::BindingSetDesc& b)
        {
            if (a.bindings.size() != b.bindings.size())
                return false;
            for (size_t i = 0; i < a.bindings.size(); ++i)
            {
                const auto& x = a.bindings[i];
                const auto& y = b.bindings[i];
                if (x.resourceHandle != y.resourceHandle || x.slot != y.slot ||
                    x.type != y.type || x.dimension != y.dimension || x.format != y.format)
                    return false;
            }
            return true;
        }
    };
};
